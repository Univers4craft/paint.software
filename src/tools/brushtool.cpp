#include "brushtool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include "core/layer.h"
#include <QPainter>
#include <cmath>

void BrushTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_drawing = true;
    m_lastPos = canvasPos;
    m_currentPos = canvasPos;
    m_beforeImage = layer->image().copy();

    // The stroke is accumulated at full opacity in its own buffer, then
    // composited onto the layer once at the tool opacity. This keeps the whole
    // stroke at a uniform opacity instead of darkening where dabs overlap.
    m_strokeBuffer = QImage(layer->image().size(), QImage::Format_ARGB32_Premultiplied);
    m_strokeBuffer.fill(Qt::transparent);

    m_strokeColor = (event->button() == Qt::LeftButton) ? doc->primaryColor() : doc->secondaryColor();
    drawBrushDab(canvasPos, m_strokeColor);
    compositeStroke(doc, layer, m_strokeColor);
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

void BrushTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    auto *layer = canvas.document()->activeLayer();
    if (layer && layer->image() != m_beforeImage)   // skip no-op strokes (no empty undo step)
        canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Paintbrush");
    m_strokeBuffer = QImage();
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

    if (m_hardness >= 95) {
        painter.setBrush(c);
        painter.drawEllipse(pos, m_brushSize / 2.0, m_brushSize / 2.0);
    } else {
        QRadialGradient gradient(pos, m_brushSize / 2.0);
        double hardRatio = m_hardness / 100.0;
        gradient.setColorAt(0, c);
        gradient.setColorAt(hardRatio, c);
        QColor transparent = c;
        transparent.setAlpha(0);
        gradient.setColorAt(1, transparent);
        painter.setBrush(gradient);
        painter.drawEllipse(pos, m_brushSize / 2.0, m_brushSize / 2.0);
    }
}

void BrushTool::compositeStroke(Document *doc, Layer *layer, const QColor &color) {
    if (m_blendMode == 14) {   // Overwrite needs a per-pixel replace, not a blend.
        compositeOverwrite(doc, layer, color);
        return;
    }
    // layer = before + strokeBuffer (at tool opacity * pressure), clipped to selection.
    QImage result = m_beforeImage.copy();
    QPainter painter(&result);
    clipToSelection(painter, doc);
    painter.setOpacity((m_opacity / 100.0) * m_pressure * (color.alphaF()));
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
    QImage result = m_beforeImage.convertToFormat(QImage::Format_ARGB32);
    const int w = result.width(), h = result.height();
    const double strength = (m_opacity / 100.0) * m_pressure;
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
    // Maps the blend-mode combo (Layer::allBlendModes order) to a Qt mode.
    switch (m_blendMode) {
    case 1:  return QPainter::CompositionMode_Multiply;   // Multiply
    case 2:  return QPainter::CompositionMode_Plus;        // Additive
    case 3:  return QPainter::CompositionMode_ColorBurn;
    case 4:  return QPainter::CompositionMode_ColorDodge;
    case 5:  return QPainter::CompositionMode_Darken;
    case 6:  return QPainter::CompositionMode_Difference;
    case 7:  return QPainter::CompositionMode_ColorDodge;  // Glow    (approximation)
    case 8:  return QPainter::CompositionMode_Lighten;
    case 9:  return QPainter::CompositionMode_Difference;  // Negation (approximation)
    case 10: return QPainter::CompositionMode_Overlay;
    case 11: return QPainter::CompositionMode_Screen;      // Reflect (approximation)
    case 12: return QPainter::CompositionMode_Screen;
    case 13: return QPainter::CompositionMode_Xor;         // Xor
    // 14 (Overwrite) is handled separately in compositeOverwrite().
    default: return QPainter::CompositionMode_SourceOver;  // Normal + unsupported
    }
}

void BrushTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    QPointF widgetPos = canvas.canvasToWidget(m_currentPos);
    double radius = m_brushSize / 2.0 * canvas.zoom();
    painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(widgetPos, radius, radius);
}
