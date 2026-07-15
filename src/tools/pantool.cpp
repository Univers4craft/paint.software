#include "pantool.h"
#include "../canvas/canvaswidget.h"
#include <QMouseEvent>

void PanTool::mousePressEvent(const QPointF &, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() == Qt::LeftButton) {
        m_panning = true;
        m_lastPos = event->pos();
        m_startPan = canvas.pan();
        canvas.setCursor(Qt::ClosedHandCursor);
    }
}

void PanTool::mouseMoveEvent(const QPointF &, QMouseEvent *event, CanvasWidget &canvas) {
    if (m_panning && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - m_lastPos;
        canvas.setPan(canvas.pan() + QPointF(delta));
        m_lastPos = event->pos();
    }
}

void PanTool::mouseReleaseEvent(const QPointF &, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        canvas.setCursor(Qt::OpenHandCursor);
    }
}
