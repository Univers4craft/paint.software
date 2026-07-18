#include "selectiontool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>

SelectionTool::SelectionTool(SelectionShape shape) : m_shape(shape) {}

void SelectionTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;
    m_selecting = true;
    m_startPos = canvasPos;
    m_currentPos = canvasPos;
}

void SelectionTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &) {
    if (!m_selecting) return;
    m_currentPos = canvasPos;
}

void SelectionTool::mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (!m_selecting) return;
    m_selecting = false;
    m_currentPos = canvasPos;

    auto *doc = canvas.document();
    if (!doc) return;

    QRect rect = QRect(toPixelPos(m_startPos), toPixelPos(m_currentPos)).normalized();
    if (rect.width() < 2 && rect.height() < 2) {
        doc->selection().clear();
    } else {
        const SelectionMode mode = selectionModeFor(event->modifiers());

        if (m_shape == SelectionShape::Rectangle)
            doc->selection().selectRect(rect, mode);
        else
            doc->selection().selectEllipse(rect, mode);
    }
    emit doc->selectionChanged();
}

void SelectionTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    if (!m_selecting) return;

    QPointF wStart = canvas.canvasToWidget(m_startPos);
    QPointF wEnd = canvas.canvasToWidget(m_currentPos);
    QRectF rect(wStart, wEnd);

    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(Qt::NoBrush);
    if (m_shape == SelectionShape::Rectangle)
        painter.drawRect(rect.normalized());
    else
        painter.drawEllipse(rect.normalized());

    painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
    if (m_shape == SelectionShape::Rectangle)
        painter.drawRect(rect.normalized());
    else
        painter.drawEllipse(rect.normalized());
}
