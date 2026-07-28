#include "moveselectiontool.h"
#include "../canvas/canvaswidget.h"
#include "../core/document.h"
#include "../core/layer.h"
#include "selectionhandles.h"
#include <QPainter>
#include <QMouseEvent>
#include <QTransform>
#include <algorithm>
#include <cmath>

void MoveSelectionTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;

    Document *doc = canvas.document();
    if (!doc || !doc->selection().hasSelection()) return;

    m_originalSelectionRect = doc->selection().boundingRect();
    m_previewSelectionRect = m_originalSelectionRect;
    m_rotCenter = QRectF(m_originalSelectionRect).center();
    m_angle = 0.0;
    m_activeHandle = SelHandles::at(m_originalSelectionRect, canvasPos, canvas.zoom());

    // Resizing a handle only rescales the marquee (this tool never moves pixels),
    // so it doesn't need an unlocked layer.
    if (m_activeHandle != SelHandles::Handle::None) {
        m_resizing = true;
        m_startPos = canvasPos;
        m_originalSelectionMask = doc->selection().mask().copy();
        return;
    }

    // Just outside the box (or a right drag) rotates the marquee about its centre.
    if (event->button() == Qt::RightButton
        || SelHandles::inRotationZone(m_originalSelectionRect, canvasPos, canvas.zoom())) {
        m_rotating = true;
        m_originalSelectionMask = doc->selection().mask().copy();
        m_rotStartAngle = std::atan2(canvasPos.y() - m_rotCenter.y(),
                                     canvasPos.x() - m_rotCenter.x()) * 180.0 / M_PI;
        return;
    }

    if (doc->selection().isSelected(canvasPos.x(), canvasPos.y())) {
        m_moving = true;
        m_lastPos = canvasPos;
    }
}

void MoveSelectionTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (m_rotating && (event->buttons() & (Qt::LeftButton | Qt::RightButton))) {
        Document *doc = canvas.document();
        if (!doc) return;
        double deg = std::atan2(canvasPos.y() - m_rotCenter.y(),
                                canvasPos.x() - m_rotCenter.x()) * 180.0 / M_PI - m_rotStartAngle;
        if (event->modifiers() & Qt::ShiftModifier)
            deg = std::round(deg / 15.0) * 15.0;
        m_angle = deg;
        SelHandles::setRotatedMask(doc->selection(), m_originalSelectionMask, m_rotCenter, m_angle, false);
        emit doc->selectionChanged();
        canvas.update();
        return;
    }

    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        m_previewSelectionRect = SelHandles::resized(m_originalSelectionRect, m_activeHandle,
                                                     canvasPos, event->modifiers());
        canvas.update();
        return;
    }

    if (m_moving && (event->buttons() & Qt::LeftButton)) {
        Document *doc = canvas.document();
        if (!doc) return;
        // A movement delta, not an absolute position: round to nearest, not floor.
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
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;

    if (m_rotating) {
        Document *doc = canvas.document();
        if (doc) {
            // Redo the rotation with a smooth mask now the angle is final.
            SelHandles::setRotatedMask(doc->selection(), m_originalSelectionMask,
                                       m_rotCenter, m_angle, true);
            emit doc->selectionChanged();
            canvas.update();
        }
        m_rotating = false;
        m_activeHandle = SelHandles::Handle::None;
        return;
    }

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

    if (m_rotating) {
        if (m_originalSelectionRect.isEmpty()) return;
        const QTransform rot = SelHandles::rotationAbout(m_rotCenter, m_angle);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        QPolygonF box;
        for (const QPointF &c : { QPointF(m_originalSelectionRect.topLeft()),
                                  QPointF(m_originalSelectionRect.topRight()),
                                  QPointF(m_originalSelectionRect.bottomRight()),
                                  QPointF(m_originalSelectionRect.bottomLeft()) })
            box << canvas.canvasToWidget(rot.map(c));
        painter.setPen(QPen(QColor(30, 30, 30), 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(box);
        painter.setPen(QPen(Qt::white, 1));
        painter.setBrush(QColor(35, 120, 255));
        painter.drawEllipse(canvas.canvasToWidget(m_rotCenter), 4, 4);
        painter.restore();
        return;
    }

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
