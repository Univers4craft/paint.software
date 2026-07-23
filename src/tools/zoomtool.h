#pragma once
#include "tool.h"

class ZoomTool : public Tool {
public:
    ToolType type() const override { return ToolType::Zoom; }
    QString name() const override { return "Zoom / Loupe"; }
    QCursor cursor() const override;

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

private:
    bool m_dragging = false;
    QPointF m_startCanvas;    // drag anchor, in canvas coordinates
    QPointF m_currentCanvas;
};
