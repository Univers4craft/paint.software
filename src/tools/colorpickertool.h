#pragma once
#include "tool.h"

class ColorPickerTool : public Tool {
public:
    ToolType type() const override { return ToolType::ColorPicker; }
    QString name() const override { return "Color Picker"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    void pickColor(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas);
};
