#include "movetool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>

void MoveTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_moving = true;
    m_startPos = canvasPos;
    m_lastDelta = QPoint(0, 0);

    if (doc->selection().hasSelection()) {
        // Move ONLY the selected pixels: lift them onto a floating copy and leave a
        // transparent hole behind (Paint.NET's "Move Selected Pixels").
        m_movingSelection = true;
        m_originalImage = layer->image().copy();
        m_floating = doc->selection().getMaskedImage(m_originalImage);   // selected pixels only
        m_hole = m_originalImage.copy();
        doc->selection().eraseFromImage(m_hole);                          // clear the selected region
    } else {
        // No selection: move the whole layer via its offset (baked in on release).
        m_movingSelection = false;
        m_originalOffset = layer->offset();
    }
}

void MoveTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_moving) return;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer) return;

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
