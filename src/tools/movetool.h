#pragma once
#include "tool.h"
#include <QImage>

class MoveTool : public Tool {
public:
    ToolType type() const override { return ToolType::Move; }
    QString name() const override { return "Move"; }
    QCursor cursor() const override { return Qt::SizeAllCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    bool m_moving = false;
    QPointF m_startPos;
    QPoint m_originalOffset;
};
