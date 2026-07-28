#include "filltool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <QQueue>
#include <cmath>

QCursor FillTool::cursor() const {
    return Qt::CrossCursor;
}

void FillTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer || layer->isLocked()) return;

    QPoint pos = toPixelPos(canvasPos);
    if (pos.x() < 0 || pos.y() < 0 || pos.x() >= layer->width() || pos.y() >= layer->height()) return;
    if (!selectionAllows(doc, pos.x(), pos.y())) return;

    QImage before = layer->image().copy();
    QColor fillColor = (event->button() == Qt::LeftButton) ? doc->primaryColor() : doc->secondaryColor();
    fillColor.setAlphaF(fillColor.alphaF() * (m_opacity / 100.0));

    // The region (which pixels to fill) is decided from the sampled source:
    // the composite of visible layers (Sampling=Image) or the active layer only
    // (Sampling=Layer). The paint is always written to the active layer.
    QImage sampleSrc = m_sampleImage ? doc->flattenVisible() : layer->image();

    // Shift temporarily forces Global mode for this click (Paint.NET behaviour).
    const bool global = m_global || (event->modifiers() & Qt::ShiftModifier);
    floodFill(layer->image(), sampleSrc, pos, fillColor, doc, global);
    doc->pushImageEdit(doc->activeLayerIndex(), before, "Paint Bucket");
}

void FillTool::mouseMoveEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}
void FillTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}

void FillTool::floodFill(QImage &target, const QImage &sampleSrc, const QPoint &pos,
                         const QColor &fillColor, Document *doc, bool global) {
    const int w = target.width(), h = target.height();

    // Region test reads the sampled source in straight-alpha space. The source
    // is normally the same size as the layer (both are document-sized), but clamp
    // reads defensively so a smaller source never overruns.
    QImage src = sampleSrc.convertToFormat(QImage::Format_ARGB32);
    const int sw = src.width(), sh = src.height();
    auto srcAt = [&](int x, int y) -> QRgb {
        if (x >= sw || y >= sh) return 0u;
        return reinterpret_cast<const QRgb*>(src.constScanLine(y))[x];
    };

    QRgb targetColor = srcAt(pos.x(), pos.y());
    const int tr = qRed(targetColor), tg = qGreen(targetColor),
              tb = qBlue(targetColor), ta = qAlpha(targetColor);
    auto within = [&](QRgb c) {
        int dr = qRed(c) - tr, dg = qGreen(c) - tg, db = qBlue(c) - tb, da = qAlpha(c) - ta;
        int dist = static_cast<int>(std::sqrt(double(dr*dr + dg*dg + db*db + da*da)));
        return dist <= toleranceDistance();
    };

    // Build the fill mask (which pixels to paint) from the sampled source.
    QVector<QVector<bool>> mask(h, QVector<bool>(w, false));
    if (global) {
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (selectionAllows(doc, x, y) && within(srcAt(x, y)))
                    mask[y][x] = true;
    } else {
        QVector<QVector<bool>> visited(h, QVector<bool>(w, false));
        QQueue<QPoint> queue;
        queue.enqueue(pos);
        visited[pos.y()][pos.x()] = true;
        while (!queue.isEmpty()) {
            QPoint p = queue.dequeue();
            if (selectionAllows(doc, p.x(), p.y()))
                mask[p.y()][p.x()] = true;
            const QPoint neighbors[] = {{p.x()-1,p.y()},{p.x()+1,p.y()},{p.x(),p.y()-1},{p.x(),p.y()+1}};
            for (const QPoint &n : neighbors) {
                if (n.x() >= 0 && n.x() < w && n.y() >= 0 && n.y() < h && !visited[n.y()][n.x()]
                    && selectionAllows(doc, n.x(), n.y())) {
                    if (within(srcAt(n.x(), n.y()))) {
                        visited[n.y()][n.x()] = true;
                        queue.enqueue(n);
                    }
                }
            }
        }
    }

    // Overwrite (blend index 14) replaces the destination pixel outright (colour
    // AND alpha), so it must be done per-pixel — drawing a buffer with
    // CompositionMode_Source would wipe untouched pixels.
    if (m_blendMode == 14) {
        QImage result = target.convertToFormat(QImage::Format_ARGB32);
        const double cov = fillColor.alphaF();       // full coverage * opacity
        const double ic = 1.0 - cov;
        const int fr = fillColor.red(), fg = fillColor.green(), fb = fillColor.blue();
        const double fa = fillColor.alphaF() * 255.0;
        for (int y = 0; y < h; ++y) {
            QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
            for (int x = 0; x < w; ++x) {
                if (!mask[y][x]) continue;
                QRgb d = dst[x];
                dst[x] = qRgba(
                    qBound(0, int(qRed(d)   * ic + fr * cov + 0.5), 255),
                    qBound(0, int(qGreen(d) * ic + fg * cov + 0.5), 255),
                    qBound(0, int(qBlue(d)  * ic + fb * cov + 0.5), 255),
                    qBound(0, int(qAlpha(d) * ic + fa * cov + 0.5), 255));
            }
        }
        target = result.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        return;
    }

    // Paint the masked fill colour into a buffer, then composite it onto the
    // active layer with the chosen blend mode — the same mechanism the brush
    // uses (opacity carries the fill alpha; the buffer holds full coverage).
    QImage buffer(w, h, QImage::Format_ARGB32);
    buffer.fill(Qt::transparent);
    const QRgb solid = qRgba(fillColor.red(), fillColor.green(), fillColor.blue(), 255);
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(buffer.scanLine(y));
        for (int x = 0; x < w; ++x)
            if (mask[y][x]) line[x] = solid;
    }

    QImage result = target.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&result);
    painter.setOpacity(fillColor.alphaF());
    painter.setCompositionMode(Tool::compositionModeFor(m_blendMode));
    painter.drawImage(0, 0, buffer);
    painter.end();
    target = result;
}
