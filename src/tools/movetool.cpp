#include "movetool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <QTransform>
#include <cmath>

void MoveTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    // Left drags move/resize; a right drag rotates the lifted content (Paint.NET).
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_moving = true;
    m_startPos = canvasPos;
    m_lastDelta = QPoint(0, 0);
    m_resizing = false;
    m_rotating = false;
    m_angle = 0.0;
    m_handle = SelHandles::Handle::None;

    if (doc->selection().hasSelection()) {
        // Move ONLY the selected pixels: lift them onto a floating copy and leave a
        // transparent hole behind (Paint.NET's "Move Selected Pixels").
        m_movingSelection = true;
        m_copyMode = (event->modifiers() & Qt::ControlModifier);   // Ctrl = move a copy
        m_originalImage = layer->image().copy();
        m_floating = doc->selection().getMaskedImage(m_originalImage);   // selected pixels only
        m_hole = m_originalImage.copy();
        // Leaving the hole intact keeps the originals in place, so only the copy moves.
        if (!m_copyMode) doc->selection().eraseFromImage(m_hole);

        m_originalRect = doc->selection().boundingRect();
        m_originalMask = doc->selection().mask().copy();
        m_rotCenter = QRectF(m_originalRect).center();

        // A handle rescales the lifted pixels; the corridor just outside the box (or
        // a right drag) rotates them; anywhere else inside moves them.
        m_handle = SelHandles::at(m_originalRect, canvasPos, canvas.zoom());
        if (m_handle != SelHandles::Handle::None) {
            m_resizing = true;
            m_previewRect = m_originalRect;
            m_floatingCrop = m_floating.copy(m_originalRect);
        } else if (event->button() == Qt::RightButton
                   || SelHandles::inRotationZone(m_originalRect, canvasPos, canvas.zoom())) {
            m_rotating = true;
            m_rotStartAngle = std::atan2(canvasPos.y() - m_rotCenter.y(),
                                         canvasPos.x() - m_rotCenter.x()) * 180.0 / M_PI;
        }
    } else {
        // No selection: move the whole layer via its offset (baked in on release).
        m_movingSelection = false;
        m_copyMode = false;
        m_originalOffset = layer->offset();
    }
}

void MoveTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (!m_moving) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer) return;

    if (m_rotating) {
        double deg = std::atan2(canvasPos.y() - m_rotCenter.y(),
                                canvasPos.x() - m_rotCenter.x()) * 180.0 / M_PI - m_rotStartAngle;
        if (event->modifiers() & Qt::ShiftModifier)
            deg = std::round(deg / 15.0) * 15.0;   // snap to 15°
        m_angle = deg;

        // Redraw the layer = hole + the lifted pixels rotated about the box centre.
        QImage result = m_hole.copy();
        {
            QPainter p(&result);
            p.setTransform(SelHandles::rotationAbout(m_rotCenter, m_angle));
            p.drawImage(0, 0, m_floating);
        }
        layer->setImage(result);

        // Rotate the marquee to match (fast while dragging).
        SelHandles::setRotatedMask(doc->selection(), m_originalMask, m_rotCenter, m_angle, false);
        emit doc->selectionChanged();
        canvas.updateCanvas();
        return;
    }

    if (m_resizing) {
        m_previewRect = SelHandles::resized(m_originalRect, m_handle, canvasPos, event->modifiers());

        // Redraw the layer = hole + the lifted pixels stretched onto the new bounds.
        // Fast scaling while dragging; the release does one smooth pass.
        QImage result = m_hole.copy();
        if (!m_floatingCrop.isNull()) {
            const QImage scaled = m_floatingCrop.scaled(m_previewRect.size(), Qt::IgnoreAspectRatio,
                                                        Qt::FastTransformation);
            QPainter p(&result);
            p.drawImage(m_previewRect.topLeft(), scaled);
            p.end();
        }
        layer->setImage(result);

        // Keep the marquee wrapped around the artwork as it is stretched.
        SelHandles::setScaledMask(doc->selection(), m_originalMask, m_originalRect, m_previewRect, false);
        emit doc->selectionChanged();
        canvas.updateCanvas();
        return;
    }

    // A movement delta, not an absolute position: round to nearest, not floor.
    // floor biases sub-pixel drags (a -0.001px nudge would jump a whole pixel up/left).
    const QPoint delta = (canvasPos - m_startPos).toPoint();

    if (m_movingSelection) {
        // Recompose the layer = hole + floating pixels shifted by delta.
        QImage result = m_hole.copy();
        QPainter p(&result);
        p.drawImage(delta, m_floating);
        p.end();
        layer->setImage(result);

        // Drag the marquee along with the pixels (incremental keeps it exact).
        if (delta != m_lastDelta) {
            doc->selection().translate(delta - m_lastDelta);
            m_lastDelta = delta;
            emit doc->selectionChanged();
        }
        canvas.updateCanvas();
    } else {
        layer->setOffset(m_originalOffset + delta);
        canvas.updateCanvas();
    }
}

void MoveTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_moving) return;
    m_moving = false;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer) return;

    if (m_rotating) {
        m_rotating = false;
        m_movingSelection = false;
        const bool changed = (std::abs(m_angle) > 1e-6);
        if (changed) {
            // Redo the rotation smoothly now the angle is final.
            QImage result = m_hole.copy();
            {
                QPainter p(&result);
                p.setRenderHint(QPainter::SmoothPixmapTransform);
                p.setTransform(SelHandles::rotationAbout(m_rotCenter, m_angle));
                p.drawImage(0, 0, m_floating);
            }
            layer->setImage(result);
            SelHandles::setRotatedMask(doc->selection(), m_originalMask, m_rotCenter, m_angle, true);
            doc->pushImageEdit(doc->activeLayerIndex(), m_originalImage,
                               m_copyMode ? "Rotate Copy" : "Rotate Selection");
            emit doc->selectionChanged();
            emit doc->documentChanged();
        } else {
            layer->setImage(m_originalImage);   // no real rotation: put the pixels back
        }
        m_floating = m_hole = m_originalImage = m_floatingCrop = m_originalMask = QImage();
        m_handle = SelHandles::Handle::None;
        return;
    }

    if (m_resizing) {
        m_resizing = false;
        m_movingSelection = false;
        const bool changed = (m_previewRect != m_originalRect);
        if (changed) {
            // Redo the scale smoothly now the size is final — the drag used a fast,
            // blocky scale to stay responsive.
            QImage result = m_hole.copy();
            if (!m_floatingCrop.isNull()) {
                const QImage scaled = m_floatingCrop.scaled(m_previewRect.size(), Qt::IgnoreAspectRatio,
                                                            Qt::SmoothTransformation);
                QPainter p(&result);
                p.drawImage(m_previewRect.topLeft(), scaled);
                p.end();
            }
            layer->setImage(result);
            SelHandles::setScaledMask(doc->selection(), m_originalMask, m_originalRect, m_previewRect, true);
            doc->pushImageEdit(doc->activeLayerIndex(), m_originalImage, "Resize Selected Pixels");
            emit doc->selectionChanged();
            emit doc->documentChanged();
        } else {
            layer->setImage(m_originalImage);   // no real resize: put the pixels back
        }
        m_floating = m_hole = m_originalImage = m_floatingCrop = m_originalMask = QImage();
        m_handle = SelHandles::Handle::None;
        return;
    }

    if (m_movingSelection) {
        m_movingSelection = false;
        const bool moved = !m_lastDelta.isNull();
        // The layer already holds hole + floating@delta from the last move.
        if (moved)
            doc->pushImageEdit(doc->activeLayerIndex(), m_originalImage, "Move Selection");
        m_floating = QImage();
        m_hole = QImage();
        m_originalImage = QImage();
        if (moved) emit doc->documentChanged();
        return;
    }

    // Whole-layer move: bake the offset into the pixels and reset it to zero so
    // every other tool (which works in image space) stays aligned.
    const QPoint finalOffset = layer->offset();
    if (finalOffset == m_originalOffset) return;   // no real move

    QImage before = layer->image().copy();
    QImage moved(before.size(), QImage::Format_ARGB32_Premultiplied);
    moved.fill(Qt::transparent);
    {
        QPainter p(&moved);
        p.drawImage(finalOffset - m_originalOffset, before);
    }
    layer->setOffset(m_originalOffset);
    layer->setImage(moved);
    doc->pushImageEdit(doc->activeLayerIndex(), before, "Move Layer");
    emit doc->documentChanged();
}

void MoveTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    auto *doc = canvas.document();
    if (!doc || !doc->selection().hasSelection()) return;

    if (m_rotating) {
        // Draw the original box rotated about its centre, so the pivot and current
        // angle are visible while dragging.
        if (m_originalRect.isEmpty()) return;
        const QTransform rot = SelHandles::rotationAbout(m_rotCenter, m_angle);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        QPolygonF box;
        for (const QPointF &c : { QPointF(m_originalRect.topLeft()), QPointF(m_originalRect.topRight()),
                                  QPointF(m_originalRect.bottomRight()), QPointF(m_originalRect.bottomLeft()) })
            box << canvas.canvasToWidget(rot.map(c));
        painter.setPen(QPen(QColor(35, 120, 255), 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(box);
        // Pivot dot.
        painter.setPen(QPen(Qt::white, 1));
        painter.setBrush(QColor(35, 120, 255));
        painter.drawEllipse(canvas.canvasToWidget(m_rotCenter), 4, 4);
        painter.restore();
        return;
    }

    const QRect rect = m_resizing ? m_previewRect : doc->selection().boundingRect();
    if (rect.isEmpty()) return;

    // Show the same handles as Move Selection, so it is visible that a corner can
    // be grabbed to stretch the artwork — without them the tool looked move-only.
    painter.save();
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(QColor(35, 120, 255));
    for (const QRectF &handle : SelHandles::rects(rect, canvas.zoom())) {
        const QPointF tl = canvas.canvasToWidget(handle.topLeft());
        const QPointF br = canvas.canvasToWidget(handle.bottomRight());
        painter.drawRect(QRectF(tl, br).normalized());
    }
    painter.restore();
}
