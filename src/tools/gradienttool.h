#pragma once
#include "tool.h"
#include <QImage>

enum class GradientType { Linear, LinearReflected, Radial, Conical, Diamond, Spiral };

// How the gradient behaves outside the start→end span (Paint.NET's repeat modes).
enum class GradientRepeat { None, Repeat, Reflect };

class GradientTool : public Tool {
public:
    ToolType type() const override { return ToolType::Gradient; }
    QString name() const override { return "Dégradé"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

    GradientType gradientType() const { return m_gradientType; }
    void setGradientType(GradientType t) { m_gradientType = t; }

    GradientRepeat repeatMode() const { return m_repeatMode; }
    void setRepeatMode(GradientRepeat r) { m_repeatMode = r; }

    bool transparencyMode() const { return m_transparencyMode; }
    void setTransparencyMode(bool on) { m_transparencyMode = on; }

private:
    void applyGradient(CanvasWidget &canvas);

    bool m_drawing = false;
    bool m_useRight = false;   // right-button drag swaps primary/secondary roles
    QPointF m_startPos, m_endPos;
    QImage m_beforeImage;
    GradientType m_gradientType = GradientType::Linear;
    GradientRepeat m_repeatMode = GradientRepeat::None;
    bool m_transparencyMode = false;
};
