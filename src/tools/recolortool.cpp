#include "recolortool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <cmath>

void RecolorTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (!canvas.document() || !canvas.document()->activeLayer()) return;
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;

    auto *layer = canvas.document()->activeLayer();
    if (layer->isLocked()) return;
    m_drawing = true;
    m_beforeImage = layer->image().copy();

    // Sample the colour under the initial click as the recolor target. Paint in
    // the primary colour with the left button, the secondary with the right —
    // Paint.NET reverses the roles of the two colours on right-click.
    const bool rightButton = event->button() == Qt::RightButton;
    QPoint c = toPixelPos(canvasPos);
    QImage &img = layer->image();
    auto *doc = canvas.document();
    m_targetColor = img.rect().contains(c) ? img.pixelColor(c)
                                           : (rightButton ? doc->primaryColor() : doc->secondaryColor());
    m_replaceColor = rightButton ? doc->secondaryColor() : doc->primaryColor();
    recolorAt(canvasPos, canvas);
}

void RecolorTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    recolorAt(canvasPos, canvas);
}

void RecolorTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    if (canvas.document())
        canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Recolor");
}

void RecolorTool::recolorAt(const QPointF &pos, CanvasWidget &canvas) {
    auto *doc = canvas.document();
    if (!doc || !doc->activeLayer()) return;

    QImage &img = doc->activeLayer()->image();
    const QColor target = m_targetColor;
    const QColor primary = m_replaceColor;
    int radius = m_brushSize / 2;
    int tol = toleranceDistance();
    QPoint center = toPixelPos(pos);

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy > radius * radius) continue;
            int px = center.x() + dx, py = center.y() + dy;
            if (px < 0 || px >= img.width() || py < 0 || py >= img.height()) continue;
            if (!selectionAllows(doc, px, py)) continue;

            QColor pixel = img.pixelColor(px, py);  // straight-alpha, correct
            int diff = std::abs(pixel.red() - target.red()) +
                       std::abs(pixel.green() - target.green()) +
                       std::abs(pixel.blue() - target.blue());
            if (diff <= tol * 3) {
                double factor = (tol > 0 && diff > 0) ? 1.0 - (double)diff / (tol * 3.0) : 1.0;
                // Tool opacity (and pen pressure) scale how strongly the new
                // colour replaces the old one.
                factor *= (m_opacity / 100.0) * m_pressure;
                int r = qBound(0, (int)(primary.red() * factor + pixel.red() * (1 - factor)), 255);
                int g = qBound(0, (int)(primary.green() * factor + pixel.green() * (1 - factor)), 255);
                int b = qBound(0, (int)(primary.blue() * factor + pixel.blue() * (1 - factor)), 255);
                img.setPixelColor(px, py, QColor(r, g, b, pixel.alpha()));
            }
        }
    }
}
