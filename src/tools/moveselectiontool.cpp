#include "moveselectiontool.h"
#include "../canvas/canvaswidget.h"
#include "../core/document.h"
#include "../core/layer.h"
#include "selectionhandles.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

void MoveSelectionTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;

    Document *doc = canvas.document();
    if (!doc || !doc->selection().hasSelection()) return;

    m_originalSelectionRect = doc->selection().boundingRect();
    m_previewSelectionRect = m_originalSelectionRect;
    m_activeHandle = SelHandles::at(m_originalSelectionRect, canvasPos, canvas.zoom());

    // Resizing a handle only rescales the marquee (this tool never moves pixels),
    // so it doesn't need an unlocked layer.
    if (m_activeHandle != SelHandles::Handle::None) {
        m_resizing = true;
        m_startPos = canvasPos;
        m_originalSelectionMask = doc->selection().mask().copy();
        return;
    }

    if (doc->selection().isSelected(canvasPos.x(), canvasPos.y())) {
        m_moving = true;
        m_lastPos = canvasPos;
    }
}

void MoveSelectionTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        m_previewSelectionRect = SelHandles::resized(m_originalSelectionRect, m_activeHandle, canvasPos);
        canvas.update();
        return;
    }

    if (m_moving && (event->buttons() & Qt::LeftButton)) {
        Document *doc = canvas.document();
        if (!doc) return;
        QPoint delta = (canvasPos - m_lastPos).toPoint();
        if (!delta.isNull()) {
            doc->selection().translate(delta);
            m_lastPos = canvasPos;
            emit doc->selectionChanged();
            canvas.update();
        }
    }
}

void MoveSelectionTool::mouseReleaseEvent(const QPointF &, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;

    if (m_resizing) {
        Document *doc = canvas.document();
        if (doc && !m_originalSelectionRect.isEmpty() && !m_previewSelectionRect.isEmpty()
            && m_previewSelectionRect != m_originalSelectionRect) {
            // Marquee-only resize: rescale the selection mask, leave every pixel
            // of the layer untouched (matches this tool's "no pixel content" contract).
            SelHandles::setScaledMask(doc->selection(), m_originalSelectionMask,
                                      m_originalSelectionRect, m_previewSelectionRect, true);

            emit doc->selectionChanged();
            canvas.update();
        }

        m_resizing = false;
        m_activeHandle = SelHandles::Handle::None;
    }

    if (event->button() == Qt::LeftButton)
        m_moving = false;
}

void MoveSelectionTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    auto *doc = canvas.document();
    if (!doc || !doc->selection().hasSelection()) return;

    QRect rect = m_resizing ? m_previewSelectionRect : doc->selection().boundingRect();
    if (rect.isEmpty()) return;

    painter.save();
    QPointF topLeft = canvas.canvasToWidget(rect.topLeft());
    QPointF bottomRight = canvas.canvasToWidget(rect.bottomRight() + QPoint(1, 1));
    QRectF widgetRect(topLeft, bottomRight);

    painter.setPen(QPen(QColor(30, 30, 30), 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(widgetRect.normalized());

    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(QColor(35, 120, 255));
    for (const QRectF &handle : SelHandles::rects(rect, canvas.zoom())) {
        QPointF handleTopLeft = canvas.canvasToWidget(handle.topLeft());
        QPointF handleBottomRight = canvas.canvasToWidget(handle.bottomRight());
        painter.drawRect(QRectF(handleTopLeft, handleBottomRight).normalized());
    }
    painter.restore();
}
