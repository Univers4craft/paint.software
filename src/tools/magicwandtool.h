#pragma once
#include "tool.h"

class MagicWandTool : public Tool {
public:
    ToolType type() const override { return ToolType::MagicWand; }
    QString name() const override { return "Baguette magique"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

    bool isGlobal() const { return m_global; }
    void setGlobal(bool global) { m_global = global; }

    // Sampling source (Paint.NET). true = Image (composite of visible layers),
    // false = Layer (the active layer only). Default Image, like Paint.NET.
    bool sampleImage() const { return m_sampleImage; }
    void setSampleImage(bool image) { m_sampleImage = image; }

private:
    bool m_global = false;
    bool m_sampleImage = true;
};
