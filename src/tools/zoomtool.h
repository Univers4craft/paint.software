#pragma once
#include "tool.h"

class ZoomTool : public Tool {
public:
    ToolType type() const override { return ToolType::Zoom; }
    QString name() const override { return "Zoom"; }
    QCursor cursor() const override;

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &, QMouseEvent *, CanvasWidget &) override {}
    void mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &) override {}
};
