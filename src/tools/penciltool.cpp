#include "penciltool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>

void PencilTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (!canvas.document() || !canvas.document()->activeLayer()) return;
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;

    m_drawing = true;
    m_lastPos = canvasPos.toPoint();
    auto *layer = canvas.document()->activeLayer();
    if (layer->isLocked()) { m_drawing = false; return; }
    m_beforeImage = layer->image().copy();

    // Draw single pixel
    QColor color = (event->button() == Qt::LeftButton)
        ? canvas.document()->primaryColor()
        : canvas.document()->secondaryColor();
    QImage &img = layer->image();
    if (img.rect().contains(m_lastPos)
        && selectionAllows(canvas.document(), m_lastPos.x(), m_lastPos.y()))
        img.setPixelColor(m_lastPos, color);
}

void PencilTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (!m_drawing || !canvas.document() || !canvas.document()->activeLayer()) return;

    auto *layer = canvas.document()->activeLayer();
    QColor color = (event->buttons() & Qt::LeftButton)
        ? canvas.document()->primaryColor()
        : canvas.document()->secondaryColor();

    QPoint pos = canvasPos.toPoint();
    // Bresenham line for 1px aliased drawing
    int x0 = m_lastPos.x(), y0 = m_lastPos.y();
    int x1 = pos.x(), y1 = pos.y();
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        if (x0 >= 0 && x0 < layer->image().width() && y0 >= 0 && y0 < layer->image().height()
            && selectionAllows(canvas.document(), x0, y0))
            layer->image().setPixelColor(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    m_lastPos = pos;
}

void PencilTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    if (canvas.document())
        canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Pencil");
}
