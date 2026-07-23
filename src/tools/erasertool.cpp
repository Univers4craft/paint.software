#include "erasertool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include "core/layer.h"
#include <QPainter>
#include <cmath>

void EraserTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_toSecondary = (event->button() == Qt::RightButton);
    m_drawing = true;
    m_lastPos = canvasPos;
    m_currentPos = canvasPos;
    m_beforeImage = layer->image().copy();
    m_strokeBuffer = QImage(layer->image().size(), QImage::Format_ARGB32_Premultiplied);
    m_strokeBuffer.fill(Qt::transparent);
    eraseDab(canvasPos);
    compositeErase(doc, layer);
}

void EraserTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    m_currentPos = canvasPos;
    if (!m_drawing) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer) return;
    eraseStroke(m_lastPos, canvasPos);
    m_lastPos = canvasPos;
    compositeErase(doc, layer);
}

void EraserTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    auto *layer = canvas.document()->activeLayer();
    if (layer && layer->image() != m_beforeImage)   // skip no-op strokes (no empty undo step)
        canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Eraser");
    m_strokeBuffer = QImage();
}

void EraserTool::eraseStroke(const QPointF &from, const QPointF &to) {
    double dx = to.x() - from.x();
    double dy = to.y() - from.y();
    double dist = std::sqrt(dx * dx + dy * dy);
    double spacing = std::max(1.0, m_brushSize * (m_spacing / 100.0));
    int steps = std::max(1, static_cast<int>(dist / spacing));

    for (int i = 1; i <= steps; ++i) {
        double t = (double)i / steps;
        QPointF pos(from.x() + dx * t, from.y() + dy * t);
        eraseDab(pos);
    }
}

void EraserTool::eraseDab(const QPointF &pos) {
    QPainter painter(&m_strokeBuffer);
    if (m_antialiased) painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    // Hardness controls the edge falloff, exactly like the brush: the dab's alpha
    // is the erase coverage (opacity is applied when the buffer is composited).
    if (m_hardness >= 95) {
        painter.setBrush(QColor(0, 0, 0, 255));
    } else {
        QRadialGradient g(pos, m_brushSize / 2.0);
        const double hardRatio = m_hardness / 100.0;
        g.setColorAt(0, QColor(0, 0, 0, 255));
        g.setColorAt(hardRatio, QColor(0, 0, 0, 255));
        g.setColorAt(1, QColor(0, 0, 0, 0));
        painter.setBrush(g);
    }
    painter.drawEllipse(pos, m_brushSize / 2.0, m_brushSize / 2.0);
}

void EraserTool::compositeErase(Document *doc, Layer *layer) {
    QImage result = m_beforeImage.copy();
    QPainter painter(&result);
    clipToSelection(painter, doc);
    // Both buttons erase to transparency; the right button scales the erase by
    // the secondary colour's alpha, matching Paint.NET (left uses the primary
    // colour's alpha — full here, since the eraser ignores primary RGB).
    double strength = (m_opacity / 100.0) * m_pressure;
    if (m_toSecondary)
        strength *= doc->secondaryColor().alphaF();
    painter.setOpacity(strength);
    // DestinationOut subtracts (strokeCoverage * strength) of alpha.
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    painter.drawImage(0, 0, m_strokeBuffer);
    painter.end();
    layer->setImage(result);
}

void EraserTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    QPointF widgetPos = canvas.canvasToWidget(m_currentPos);
    double radius = m_brushSize / 2.0 * canvas.zoom();
    painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(widgetPos, radius, radius);
}
