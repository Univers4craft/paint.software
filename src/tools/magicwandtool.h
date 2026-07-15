#pragma once
#include "tool.h"

class MagicWandTool : public Tool {
public:
    ToolType type() const override { return ToolType::MagicWand; }
    QString name() const override { return "Magic Wand"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

    bool isGlobal() const { return m_global; }
    void setGlobal(bool global) { m_global = global; }

private:
    bool m_global = false;
};
