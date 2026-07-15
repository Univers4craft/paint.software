#include "moveselectiontool.h"
#include "../canvas/canvaswidget.h"
#include "../core/document.h"
#include "../core/layer.h"
#include <QPainter>
#include <QMouseEvent>

void MoveSelectionTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;

    Document *doc = canvas.document();
    if (!doc || !doc->selection().hasSelection()) return;

    m_originalSelectionRect = doc->selection().boundingRect();
    m_previewSelectionRect = m_originalSelectionRect;
    m_activeHandle = handleAt(m_originalSelectionRect, canvasPos);

    // Resizing a handle only rescales the marquee (this tool never moves pixels),
    // so it doesn't need an unlocked layer.
    if (m_activeHandle != ResizeHandle::None) {
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
        m_previewSelectionRect = resizedRect(m_originalSelectionRect, m_activeHandle, canvasPos);
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
            QImage newMask(doc->selection().mask().size(), QImage::Format_Grayscale8);
            newMask.fill(0);
            QImage scaledMask = m_originalSelectionMask.copy(m_originalSelectionRect)
                .scaled(m_previewSelectionRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                .convertToFormat(QImage::Format_Grayscale8);
            QPainter maskPainter(&newMask);
            maskPainter.drawImage(m_previewSelectionRect.topLeft(), scaledMask);
            maskPainter.end();
            doc->selection().setMaskImage(newMask);

            emit doc->selectionChanged();
            canvas.update();
        }

        m_resizing = false;
        m_activeHandle = ResizeHandle::None;
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
    for (const QRectF &handle : handleRects(rect)) {
        QPointF handleTopLeft = canvas.canvasToWidget(handle.topLeft());
        QPointF handleBottomRight = canvas.canvasToWidget(handle.bottomRight());
        painter.drawRect(QRectF(handleTopLeft, handleBottomRight).normalized());
    }
    painter.restore();
}

MoveSelectionTool::ResizeHandle MoveSelectionTool::handleAt(const QRect &rect, const QPointF &canvasPos) const {
    if (rect.isEmpty()) return ResizeHandle::None;

    const int handleRadius = 6;
    const QPoint points[] = {
        rect.topLeft(),
        QPoint(rect.center().x(), rect.top()),
        rect.topRight(),
        QPoint(rect.right(), rect.center().y()),
        rect.bottomRight(),
        QPoint(rect.center().x(), rect.bottom()),
        rect.bottomLeft(),
        QPoint(rect.left(), rect.center().y())
    };
    const ResizeHandle handles[] = {
        ResizeHandle::TopLeft,
        ResizeHandle::Top,
        ResizeHandle::TopRight,
        ResizeHandle::Right,
        ResizeHandle::BottomRight,
        ResizeHandle::Bottom,
        ResizeHandle::BottomLeft,
        ResizeHandle::Left
    };

    for (int i = 0; i < 8; ++i) {
        QRect hit(points[i].x() - handleRadius, points[i].y() - handleRadius, handleRadius * 2 + 1, handleRadius * 2 + 1);
        if (hit.contains(canvasPos.toPoint())) {
            return handles[i];
        }
    }
    return ResizeHandle::None;
}

QRect MoveSelectionTool::resizedRect(const QRect &rect, ResizeHandle handle, const QPointF &canvasPos) const {
    QRect result = rect;
    const QPoint pos = canvasPos.toPoint();

    switch (handle) {
    case ResizeHandle::TopLeft:
        result.setTopLeft(pos);
        break;
    case ResizeHandle::Top:
        result.setTop(pos.y());
        break;
    case ResizeHandle::TopRight:
        result.setTopRight(pos);
        break;
    case ResizeHandle::Right:
        result.setRight(pos.x());
        break;
    case ResizeHandle::BottomRight:
        result.setBottomRight(pos);
        break;
    case ResizeHandle::Bottom:
        result.setBottom(pos.y());
        break;
    case ResizeHandle::BottomLeft:
        result.setBottomLeft(pos);
        break;
    case ResizeHandle::Left:
        result.setLeft(pos.x());
        break;
    case ResizeHandle::None:
        break;
    }

    return normalizedWithMinimumSize(result);
}

QRect MoveSelectionTool::normalizedWithMinimumSize(const QRect &rect) const {
    QRect normalized = rect.normalized();
    if (normalized.width() < 2) normalized.setWidth(2);
    if (normalized.height() < 2) normalized.setHeight(2);
    return normalized;
}

QVector<QRectF> MoveSelectionTool::handleRects(const QRect &rect) const {
    const qreal size = 4.0;
    const qreal half = size / 2.0;
    return {
        QRectF(rect.left() - half, rect.top() - half, size, size),
        QRectF(rect.center().x() - half, rect.top() - half, size, size),
        QRectF(rect.right() - half, rect.top() - half, size, size),
        QRectF(rect.right() - half, rect.center().y() - half, size, size),
        QRectF(rect.right() - half, rect.bottom() - half, size, size),
        QRectF(rect.center().x() - half, rect.bottom() - half, size, size),
        QRectF(rect.left() - half, rect.bottom() - half, size, size),
        QRectF(rect.left() - half, rect.center().y() - half, size, size)
    };
}
