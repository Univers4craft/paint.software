#include "selectiontool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <cmath>

SelectionTool::SelectionTool(SelectionShape shape) : m_shape(shape) {}

// Returns the constrained end-point given the drag start and raw current position.
QPointF SelectionTool::applyConstraint(const QPointF &start, const QPointF &current) const {
    if (m_constraint == SelectionConstraint::Normal)
        return current;

    const double dx = current.x() - start.x();
    const double dy = current.y() - start.y();

    if (m_constraint == SelectionConstraint::FixedSize) {
        // The selection is always exactly m_cw × m_ch; the end corner is fixed
        // relative to the start, preserving the drag quadrant.
        const double sx = dx >= 0 ? 1.0 : -1.0;
        const double sy = dy >= 0 ? 1.0 : -1.0;
        return QPointF(start.x() + sx * m_cw, start.y() + sy * m_ch);
    }

    // FixedRatio: scale the smaller axis to match the aspect ratio m_cw:m_ch,
    // always in the direction the user is dragging.
    if (qFuzzyIsNull(m_cw) || qFuzzyIsNull(m_ch))
        return current;

    const double ratio = m_cw / m_ch;
    const double absDx = std::abs(dx);
    const double absDy = std::abs(dy);

    double newDx = dx, newDy = dy;
    if (absDy * ratio > absDx)
        newDx = std::copysign(absDy * ratio, dx);
    else
        newDy = std::copysign(absDx / ratio, dy);

    return QPointF(start.x() + newDx, start.y() + newDy);
}

void SelectionTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    if (event->button() == Qt::RightButton && event->modifiers() == Qt::NoModifier) return;
    m_button = event->button();
    m_selecting = true;
    m_startPos = canvasPos;
    m_currentPos = applyConstraint(canvasPos, canvasPos);
}

void SelectionTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &) {
    if (!m_selecting) return;
    m_currentPos = applyConstraint(m_startPos, canvasPos);
}

void SelectionTool::mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (!m_selecting) return;
    m_selecting = false;
    m_currentPos = applyConstraint(m_startPos, canvasPos);

    auto *doc = canvas.document();
    if (!doc) return;

    QRect rect = QRect(toPixelPos(m_startPos), toPixelPos(m_currentPos)).normalized();
    if (rect.width() < 2 && rect.height() < 2) {
        doc->selection().clear();
    } else {
        const SelectionMode mode = selectionModeFor(event->modifiers(), m_button);
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
