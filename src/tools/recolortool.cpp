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

    // Decide the recolor target. Paint in the primary colour with the left
    // button, the secondary with the right — Paint.NET reverses the roles of the
    // two colours on right-click.
    const bool rightButton = event->button() == Qt::RightButton;
    auto *doc = canvas.document();
    if (m_sampleSecondary) {
        // Target is a fixed colour (the secondary), not the clicked pixel. On
        // right-click the roles reverse: target becomes the primary.
        m_targetColor = rightButton ? doc->primaryColor() : doc->secondaryColor();
    } else {
        QPoint c = toPixelPos(canvasPos);
        QImage &img = layer->image();
        m_targetColor = img.rect().contains(c) ? img.pixelColor(c)
                                               : (rightButton ? doc->primaryColor() : doc->secondaryColor());
    }
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
    // Paint.NET stylus model: pressure varies the tip SIZE (0..brush size), not
    // the blend strength. A mouse reports full pressure, so its tip is full size.
    int radius = int((m_brushSize / 2.0) * (m_pressureSensitivity ? m_pressure : 1.0));
    int tol = toleranceDistance();
    QPoint center = toPixelPos(pos);
    // Soft-edge falloff, like the paintbrush: full strength out to hardRatio of
    // the radius, then fading to zero at the rim. Hardness 100 = a hard tip.
    const double hardRatio = m_hardness / 100.0;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const int dist2 = dx * dx + dy * dy;
            if (dist2 > radius * radius) continue;
            int px = center.x() + dx, py = center.y() + dy;
            if (px < 0 || px >= img.width() || py < 0 || py >= img.height()) continue;
            if (!selectionAllows(doc, px, py)) continue;

            QColor pixel = img.pixelColor(px, py);  // straight-alpha, correct
            int diff = std::abs(pixel.red() - target.red()) +
                       std::abs(pixel.green() - target.green()) +
                       std::abs(pixel.blue() - target.blue());
            if (diff <= tol * 3) {
                double factor = (tol > 0 && diff > 0) ? 1.0 - (double)diff / (tol * 3.0) : 1.0;
                // Tool opacity scales how strongly the new colour replaces the
                // old one (pressure varies the tip size, not the blend).
                factor *= (m_opacity / 100.0);
                // Radial soft-edge falloff scales the replacement strength.
                if (radius > 0 && hardRatio < 1.0) {
                    const double norm = std::sqrt((double)dist2) / radius;   // 0..1
                    double falloff = 1.0;
                    if (norm > hardRatio)
                        falloff = (norm >= 1.0) ? 0.0
                                                : 1.0 - (norm - hardRatio) / (1.0 - hardRatio);
                    factor *= falloff;
                }
                int r = qBound(0, (int)(primary.red() * factor + pixel.red() * (1 - factor)), 255);
                int g = qBound(0, (int)(primary.green() * factor + pixel.green() * (1 - factor)), 255);
                int b = qBound(0, (int)(primary.blue() * factor + pixel.blue() * (1 - factor)), 255);
                img.setPixelColor(px, py, QColor(r, g, b, pixel.alpha()));
            }
        }
    }
}
