#include "brushtool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include "core/hatchpatterns.h"
#include "core/layer.h"
#include <QPainter>
#include <cmath>

void BrushTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer || layer->isLocked()) return;

    const QColor col = (event->button() == Qt::LeftButton) ? doc->primaryColor() : doc->secondaryColor();

    // A second draw-button pressed mid-stroke switches the colour without breaking
    // the line, rather than stopping or restarting the stroke (issue #17).
    if (m_drawing) {
        if (col != m_strokeColor) switchColour(doc, layer, col);
        return;
    }

    m_drawing = true;
    m_lastPos = canvasPos;
    m_currentPos = canvasPos;
    m_beforeImage = layer->image().copy();
    m_baseImage = m_beforeImage;

    // The stroke is accumulated at full opacity in its own buffer, then
    // composited onto the layer once at the tool opacity. This keeps the whole
    // stroke at a uniform opacity instead of darkening where dabs overlap.
    m_strokeBuffer = QImage(layer->image().size(), QImage::Format_ARGB32_Premultiplied);
    m_strokeBuffer.fill(Qt::transparent);

    m_strokeColor = col;
    drawBrushDab(canvasPos, m_strokeColor);
    compositeStroke(doc, layer, m_strokeColor);
}

void BrushTool::switchColour(Document *doc, Layer *layer, const QColor &color) {
    // Bake the coverage drawn so far into the base, then continue in the new
    // colour from the current point (no gap).
    m_baseImage = layer->image().copy();
    m_strokeBuffer.fill(Qt::transparent);
    m_strokeColor = color;
}

void BrushTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    m_currentPos = canvasPos;
    if (!m_drawing) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer) return;

    Q_UNUSED(event);
    drawBrushStroke(m_lastPos, canvasPos, m_strokeColor);
    m_lastPos = canvasPos;
    compositeStroke(doc, layer, m_strokeColor);
}

void BrushTool::mouseReleaseEvent(const QPointF &, QMouseEvent *event, CanvasWidget &canvas) {
    if (!m_drawing) return;
    auto *doc = canvas.document();
    auto *layer = doc ? doc->activeLayer() : nullptr;
    // Releasing one of two held buttons must not cut the stroke — keep drawing in
    // the still-held button's colour (issue #17).
    if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) {
        if (layer) {
            const QColor col = (event->buttons() & Qt::LeftButton) ? doc->primaryColor()
                                                                   : doc->secondaryColor();
            if (col != m_strokeColor) switchColour(doc, layer, col);
        }
        return;
    }
    m_drawing = false;
    if (layer && layer->image() != m_beforeImage)   // skip no-op strokes (no empty undo step)
        doc->pushImageEdit(doc->activeLayerIndex(), m_beforeImage, "Paintbrush");
    m_strokeBuffer = QImage();
    m_baseImage = QImage();
}

void BrushTool::drawBrushStroke(const QPointF &from, const QPointF &to, const QColor &color) {
    double dx = to.x() - from.x();
    double dy = to.y() - from.y();
    double dist = std::sqrt(dx * dx + dy * dy);
    double spacing = std::max(1.0, m_brushSize * (m_spacing / 100.0));
    int steps = std::max(1, static_cast<int>(dist / spacing));

    for (int i = 1; i <= steps; ++i) {
        double t = (double)i / steps;
        QPointF pos(from.x() + dx * t, from.y() + dy * t);
        drawBrushDab(pos, color);
    }
}

void BrushTool::drawBrushDab(const QPointF &pos, const QColor &color) {
    QPainter painter(&m_strokeBuffer);
    if (m_antialiased) painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    // Draw the dab opaque; opacity is applied when the buffer is composited.
    QColor c = color;
    c.setAlphaF(1.0);

    // Paint.NET model: stylus pressure varies the dab SIZE (0..brush size), not
    // the opacity. A mouse reports full pressure, so it stamps the full size.
    const double p = m_pressureSensitivity ? m_pressure : 1.0;
    const double radius = (m_brushSize / 2.0) * p;
    if (radius < 0.4) return;   // effectively no pressure = no mark

    if (m_fillStyle > 0) {
        // Patterned brush (Fill Style): stamp the hatch — pattern lines in the
        // draw colour, transparent between them — with a hard edge.
        painter.setBrush(Hatch::brush(m_fillStyle, c, QColor(Qt::transparent)));
        painter.drawEllipse(pos, radius, radius);
        return;
    }

    if (m_hardness >= 95) {
        painter.setBrush(c);
        painter.drawEllipse(pos, radius, radius);
    } else {
        QRadialGradient gradient(pos, radius);
        double hardRatio = m_hardness / 100.0;
        gradient.setColorAt(0, c);
        gradient.setColorAt(hardRatio, c);
        QColor transparent = c;
        transparent.setAlpha(0);
        gradient.setColorAt(1, transparent);
        painter.setBrush(gradient);
        painter.drawEllipse(pos, radius, radius);
    }
}

void BrushTool::compositeStroke(Document *doc, Layer *layer, const QColor &color) {
    if (m_blendMode == 14) {   // Overwrite needs a per-pixel replace, not a blend.
        compositeOverwrite(doc, layer, color);
        return;
    }
    // layer = base + strokeBuffer (at tool opacity), clipped to selection. The
    // base carries any earlier-colour segments of this same stroke.
    QImage result = m_baseImage.copy();
    QPainter painter(&result);
    clipToSelection(painter, doc);
    // Opacity no longer depends on pressure — pressure drives dab size instead.
    painter.setOpacity((m_opacity / 100.0) * (color.alphaF()));
    painter.setCompositionMode(brushCompositionMode());
    painter.drawImage(0, 0, m_strokeBuffer);
    painter.end();
    layer->setImage(result);
}

void BrushTool::compositeOverwrite(Document *doc, Layer *layer, const QColor &color) {
    // Overwrite replaces the destination pixel outright (colour AND alpha) within
    // the stroke's coverage, so it can also lower alpha. Only painted pixels are
    // touched — drawing the whole buffer with CompositionMode_Source would wipe
    // every untouched pixel of the layer (the original bug).
    QImage result = m_baseImage.convertToFormat(QImage::Format_ARGB32);
    const int w = result.width(), h = result.height();
    const double strength = (m_opacity / 100.0);   // pressure drives size, not opacity
    const int tr = color.red(), tg = color.green(), tb = color.blue();
    const double ta = color.alphaF() * 255.0;
    for (int y = 0; y < h; ++y) {
        const QRgb *buf = reinterpret_cast<const QRgb*>(m_strokeBuffer.constScanLine(y));
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double cov = (qAlpha(buf[x]) / 255.0) * strength;
            if (cov <= 0.0) continue;
            if (!selectionAllows(doc, x, y)) continue;
            const double ic = 1.0 - cov;
            QRgb d = dst[x];
            dst[x] = qRgba(
                qBound(0, int(qRed(d)   * ic + tr * cov + 0.5), 255),
                qBound(0, int(qGreen(d) * ic + tg * cov + 0.5), 255),
                qBound(0, int(qBlue(d)  * ic + tb * cov + 0.5), 255),
                qBound(0, int(qAlpha(d) * ic + ta * cov + 0.5), 255));
        }
    }
    layer->setImage(result.convertToFormat(QImage::Format_ARGB32_Premultiplied));
}

QPainter::CompositionMode BrushTool::brushCompositionMode() const {
    // Shared mapping (Layer::allBlendModes order). 14 (Overwrite) is handled
    // separately in compositeOverwrite().
    return Tool::compositionModeFor(m_blendMode);
}

void BrushTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    QPointF widgetPos = canvas.canvasToWidget(m_currentPos);
    double radius = m_brushSize / 2.0 * canvas.zoom();
    painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(widgetPos, radius, radius);
}
