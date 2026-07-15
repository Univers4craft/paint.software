#include "linetool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <cmath>

void LineTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_drawing = true;
    m_startPos = canvasPos;
    m_currentPos = canvasPos;
    m_beforeImage = layer->image().copy();
}

void LineTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_currentPos = canvasPos;

    auto *layer = canvas.document()->activeLayer();
    if (!layer) return;

    layer->setImage(m_beforeImage.copy());
    QPainter painter(&layer->image());
    if (m_antialiased) painter.setRenderHint(QPainter::Antialiasing);
    clipToSelection(painter, canvas.document());

    QColor color = (event->buttons() & Qt::LeftButton)
        ? canvas.document()->primaryColor()
        : canvas.document()->secondaryColor();

    QPen pen(color, m_brushSize, Qt::SolidLine, Qt::RoundCap);
    if (m_lineStyle == LineStyle::DashedLine) pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    painter.setOpacity(m_opacity / 100.0);
    painter.drawLine(m_startPos, m_currentPos);

    if (m_lineStyle == LineStyle::Arrow) {
        // Draw arrowhead
        double angle = std::atan2(m_currentPos.y() - m_startPos.y(), m_currentPos.x() - m_startPos.x());
        double arrowSize = m_brushSize * 3;
        QPointF p1(m_currentPos.x() - arrowSize * cos(angle - M_PI/6),
                    m_currentPos.y() - arrowSize * sin(angle - M_PI/6));
        QPointF p2(m_currentPos.x() - arrowSize * cos(angle + M_PI/6),
                    m_currentPos.y() - arrowSize * sin(angle + M_PI/6));
        painter.setBrush(color);
        QPainterPath arrow;
        arrow.moveTo(m_currentPos);
        arrow.lineTo(p1);
        arrow.lineTo(p2);
        arrow.closeSubpath();
        painter.drawPath(arrow);
    }
}

void LineTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Line");
}

void LineTool::drawOverlay(QPainter &, const CanvasWidget &) {}
