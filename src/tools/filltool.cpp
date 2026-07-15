#include "filltool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
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

    QPoint pos = canvasPos.toPoint();
    if (pos.x() < 0 || pos.y() < 0 || pos.x() >= layer->width() || pos.y() >= layer->height()) return;
    if (!selectionAllows(doc, pos.x(), pos.y())) return;

    QImage before = layer->image().copy();
    QColor fillColor = (event->button() == Qt::LeftButton) ? doc->primaryColor() : doc->secondaryColor();
    fillColor.setAlphaF(fillColor.alphaF() * (m_opacity / 100.0));

    floodFill(layer->image(), pos, fillColor, doc);
    doc->pushImageEdit(doc->activeLayerIndex(), before, "Paint Bucket");
}

void FillTool::mouseMoveEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}
void FillTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}

void FillTool::floodFill(QImage &image, const QPoint &pos, const QColor &fillColor, Document *doc) {
    // Work in straight-alpha space so colour comparisons and blending are correct
    // regardless of the layer's premultiplied storage.
    QImage buf = image.convertToFormat(QImage::Format_ARGB32);
    const int w = buf.width(), h = buf.height();

    auto colorAt = [&](int x, int y) { return reinterpret_cast<QRgb*>(buf.scanLine(y))[x]; };
    QRgb targetColor = colorAt(pos.x(), pos.y());
    int tr = qRed(targetColor), tg = qGreen(targetColor), tb = qBlue(targetColor), ta = qAlpha(targetColor);

    // Composite fill colour over an existing pixel (source-over, straight alpha).
    const int fr = fillColor.red(), fg = fillColor.green(), fb = fillColor.blue();
    const double fa = fillColor.alphaF();
    auto blended = [&](QRgb bg) -> QRgb {
        double ba = qAlpha(bg) / 255.0;
        double oa = fa + ba * (1.0 - fa);
        if (oa <= 0.0) return qRgba(0, 0, 0, 0);
        int r = int((fr * fa + qRed(bg) * ba * (1.0 - fa)) / oa + 0.5);
        int g = int((fg * fa + qGreen(bg) * ba * (1.0 - fa)) / oa + 0.5);
        int b = int((fb * fa + qBlue(bg) * ba * (1.0 - fa)) / oa + 0.5);
        return qRgba(qBound(0,r,255), qBound(0,g,255), qBound(0,b,255), int(oa * 255 + 0.5));
    };

    QVector<QVector<bool>> visited(h, QVector<bool>(w, false));
    QQueue<QPoint> queue;
    queue.enqueue(pos);
    visited[pos.y()][pos.x()] = true;

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();
        if (selectionAllows(doc, p.x(), p.y()))
            reinterpret_cast<QRgb*>(buf.scanLine(p.y()))[p.x()] = blended(colorAt(p.x(), p.y()));

        const QPoint neighbors[] = {{p.x()-1,p.y()},{p.x()+1,p.y()},{p.x(),p.y()-1},{p.x(),p.y()+1}};
        for (const QPoint &n : neighbors) {
            if (n.x() >= 0 && n.x() < w && n.y() >= 0 && n.y() < h && !visited[n.y()][n.x()]
                && selectionAllows(doc, n.x(), n.y())) {   // keep the flood inside the selection
                QRgb c = colorAt(n.x(), n.y());
                int dr = qRed(c) - tr, dg = qGreen(c) - tg, db = qBlue(c) - tb, da = qAlpha(c) - ta;
                int dist = static_cast<int>(std::sqrt(double(dr*dr + dg*dg + db*db + da*da)));
                if (dist <= m_tolerance) {
                    visited[n.y()][n.x()] = true;
                    queue.enqueue(n);
                }
            }
        }
    }

    image = buf.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}
