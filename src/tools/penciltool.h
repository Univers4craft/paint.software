#pragma once
#include "tool.h"

class PencilTool : public Tool {
public:
    ToolType type() const override { return ToolType::Pencil; }
    QString name() const override { return "Pencil"; }
    QCursor cursor() const override { return Qt::CrossCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    bool m_drawing = false;
    QPoint m_lastPos;
    QImage m_beforeImage;
};
