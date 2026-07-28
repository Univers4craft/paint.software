#include "clonestamptool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <cmath>

void CloneStampTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    auto *doc = canvas.document();
    auto *layer = doc ? doc->activeLayer() : nullptr;
    if (!layer) return;

    // Paint.NET sets the clone source with Ctrl+click. Alt and right-click are
    // kept as familiar aliases from other editors.
    if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)
        || event->button() == Qt::RightButton) {
        m_sourceSet = true;
        m_sourcePos = canvasPos;
        m_sourceImage = layer->image().copy();
        return;
    }

    if (!m_sourceSet || event->button() != Qt::LeftButton) return;
    if (layer->isLocked()) return;

    m_drawing = true;
    m_lastPos = canvasPos;
    m_currentPos = canvasPos;
    m_offset = m_sourcePos - canvasPos;
    m_beforeImage = layer->image().copy();
    m_coverage = QImage(layer->image().size(), QImage::Format_Grayscale8);
    m_coverage.fill(0);
    m_doc = doc;

    cloneDab(canvasPos, layer->image(), m_sourceImage);
}

void CloneStampTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    m_currentPos = canvasPos;
    if (!m_drawing) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer) return;

    double dx = canvasPos.x() - m_lastPos.x();
    double dy = canvasPos.y() - m_lastPos.y();
    double dist = std::sqrt(dx * dx + dy * dy);
    double spacing = std::max(1.0, m_brushSize * (m_spacing / 100.0));
    int steps = std::max(1, static_cast<int>(dist / spacing));

    for (int i = 1; i <= steps; ++i) {
        double t = (double)i / steps;
        QPointF pos(m_lastPos.x() + dx * t, m_lastPos.y() + dy * t);
        cloneDab(pos, layer->image(), m_sourceImage);
    }
    m_lastPos = canvasPos;
}

void CloneStampTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Clone Stamp");
}

void CloneStampTool::cloneDab(const QPointF &pos, QImage &target, const QImage &source) {
    // Paint.NET stylus model: pressure varies the stamp SIZE (0..brush size),
    // not opacity. A mouse reports full pressure, so its stamps are full size.
    const double p = m_pressureSensitivity ? m_pressure : 1.0;
    const double scaledRadius = (m_brushSize / 2.0) * p;
    if (scaledRadius < 0.4) return;   // effectively no pressure = no mark
    int r = std::max(1, static_cast<int>(scaledRadius));   // size 1 must not give r=0 (÷0 → NaN)
    QPointF srcPos = pos + m_offset;
    const double hard = m_hardness / 100.0;
    const double strength = (m_opacity / 100.0);

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy > r * r) continue;
            int sx = static_cast<int>(srcPos.x() + dx);
            int sy = static_cast<int>(srcPos.y() + dy);
            int tx = static_cast<int>(pos.x() + dx);
            int ty = static_cast<int>(pos.y() + dy);

            if (sx >= 0 && sx < source.width() && sy >= 0 && sy < source.height()
                && tx >= 0 && tx < target.width() && ty >= 0 && ty < target.height()) {
                if (!selectionAllows(m_doc, tx, ty)) continue;
                double dist = std::sqrt(double(dx * dx + dy * dy)) / r;
                double falloff = (m_hardness >= 95) ? 1.0
                    : std::max(0.0, 1.0 - (dist - hard) / (1.0 - hard));
                int cov = static_cast<int>(falloff * strength * 255.0 + 0.5);
                if (cov <= 0) continue;

                // Track the MAX coverage laid on each pixel this stroke and always
                // recomposite from m_beforeImage — overlapping dabs and segment
                // joins then stop darkening (no opacity buildup, no seam).
                uchar *covLine = m_coverage.scanLine(ty);
                if (cov <= covLine[tx]) continue;
                covLine[tx] = static_cast<uchar>(cov);
                double a = cov / 255.0;

                QColor srcPixel = source.pixelColor(sx, sy);        // straight-alpha
                QColor basePixel = m_beforeImage.pixelColor(tx, ty);
                int r2 = static_cast<int>(srcPixel.red()   * a + basePixel.red()   * (1 - a));
                int g  = static_cast<int>(srcPixel.green() * a + basePixel.green() * (1 - a));
                int b  = static_cast<int>(srcPixel.blue()  * a + basePixel.blue()  * (1 - a));
                int al = static_cast<int>(srcPixel.alpha() * a + basePixel.alpha() * (1 - a));
                target.setPixelColor(tx, ty, QColor(qBound(0,r2,255), qBound(0,g,255), qBound(0,b,255), qBound(0,al,255)));
            }
        }
    }
}

void CloneStampTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    double radius = m_brushSize / 2.0 * canvas.zoom();

    // Draw current brush circle
    QPointF widgetPos = canvas.canvasToWidget(m_currentPos);
    painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(widgetPos, radius, radius);

    // Draw source crosshair
    if (m_sourceSet) {
        QPointF srcWidget = canvas.canvasToWidget(m_sourcePos);
        painter.setPen(QPen(Qt::red, 1));
        painter.drawLine(QPointF(srcWidget.x() - 10, srcWidget.y()), QPointF(srcWidget.x() + 10, srcWidget.y()));
        painter.drawLine(QPointF(srcWidget.x(), srcWidget.y() - 10), QPointF(srcWidget.x(), srcWidget.y() + 10));
        painter.drawEllipse(srcWidget, radius, radius);
    }
}
