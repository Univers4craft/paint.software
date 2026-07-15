#include "movetool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>

void MoveTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_moving = true;
    m_startPos = canvasPos;
    m_originalOffset = layer->offset();
}

void MoveTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_moving) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer) return;

    QPointF delta = canvasPos - m_startPos;
    layer->setOffset(m_originalOffset + delta.toPoint());
}

void MoveTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_moving) return;
    m_moving = false;
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer) return;

    QPoint finalOffset = layer->offset();
    if (finalOffset == m_originalOffset) return;  // no real move

    // Bake the offset into the pixels and reset it to zero so every other tool
    // (which works in image space) stays aligned with what's displayed.
    QImage before = layer->image().copy();
    QImage moved(before.size(), QImage::Format_ARGB32_Premultiplied);
    moved.fill(Qt::transparent);
    {
        QPainter p(&moved);
        p.drawImage(finalOffset - m_originalOffset, before);
    }
    layer->setOffset(m_originalOffset);
    layer->setImage(moved);
    doc->pushImageEdit(doc->activeLayerIndex(), before, "Move Selection");
    emit doc->documentChanged();
}
