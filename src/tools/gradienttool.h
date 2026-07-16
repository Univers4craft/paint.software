#pragma once
#include "tool.h"
#include <QImage>

enum class GradientType { Linear, Radial, Conical, Diamond };

class GradientTool : public Tool {
public:
    ToolType type() const override { return ToolType::Gradient; }
    QString name() const override { return "Dégradé"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

    GradientType gradientType() const { return m_gradientType; }
    void setGradientType(GradientType t) { m_gradientType = t; }

private:
    void applyGradient(CanvasWidget &canvas);

    bool m_drawing = false;
    QPointF m_startPos, m_endPos;
    QImage m_beforeImage;
    GradientType m_gradientType = GradientType::Linear;
};
