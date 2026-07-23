#include "lassotool.h"
#include "../canvas/canvaswidget.h"
#include "../core/document.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

void LassoTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &) {
    // Left draws freely; right only with a modifier (Alt+right = intersect,
    // Ctrl+right = invert). Bare right-click is left for the context menu.
    const bool left = event->button() == Qt::LeftButton;
    const bool modRight = event->button() == Qt::RightButton && event->modifiers() != Qt::NoModifier;
    if (left || modRight) {
        m_selecting = true;
        m_button = event->button();
        m_modifiers = event->modifiers();
        m_lasso.clear();
        m_lasso << canvasPos;
    }
}

void LassoTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (m_selecting && (event->buttons() & (Qt::LeftButton | Qt::RightButton))) {
        m_lasso << canvasPos;
        canvas.update();
    }
}

void LassoTool::mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (m_selecting && event->button() == m_button) {
        m_lasso << canvasPos;
        m_selecting = false;

        Document *doc = canvas.document();
        if (doc && m_lasso.size() > 2) {
            QPainterPath path;
            path.addPolygon(m_lasso);
            path.closeSubpath();
            doc->selection().selectPath(path, selectionModeFor(m_modifiers, m_button));
            emit doc->selectionChanged();
        }
        m_lasso.clear();
        canvas.update();
    }
}

void LassoTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    if (!m_selecting || m_lasso.size() < 2) return;

    painter.save();
    int rs = canvas.rulerSize();
    painter.translate(rs + canvas.pan().x(), rs + canvas.pan().y());
    painter.scale(canvas.zoom(), canvas.zoom());

    QPen pen(Qt::white, 1.0 / canvas.zoom());
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(m_lasso);

    QPen dashPen(Qt::black, 1.0 / canvas.zoom(), Qt::DashLine);
    dashPen.setDashPattern({4, 4});
    painter.setPen(dashPen);
    painter.drawPolyline(m_lasso);
    painter.restore();
}
