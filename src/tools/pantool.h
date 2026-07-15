#pragma once
#include "tool.h"

class PanTool : public Tool {
public:
    ToolType type() const override { return ToolType::Pan; }
    QString name() const override { return "Main"; }
    QCursor cursor() const override { return m_panning ? Qt::ClosedHandCursor : Qt::OpenHandCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    bool m_panning = false;
    QPoint m_lastPos;
    QPointF m_startPan;
};
