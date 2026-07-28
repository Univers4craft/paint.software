#pragma once
#include "tool.h"

class RecolorTool : public Tool {
public:
    ToolType type() const override { return ToolType::Recolor; }
    QString name() const override { return "Recoloriage"; }
    QCursor cursor() const override { return Qt::CrossCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

    // Sampling (Paint.NET "Target"): false = the target colour is sampled under
    // the initial click; true = the target is a fixed colour, the document's
    // secondary colour (right-click reverses primary/secondary as usual).
    bool sampleSecondary() const { return m_sampleSecondary; }
    void setSampleSecondary(bool on) { m_sampleSecondary = on; }

private:
    void recolorAt(const QPointF &pos, CanvasWidget &canvas);
    bool m_drawing = false;
    bool m_sampleSecondary = false;   // target = secondary colour instead of clicked pixel
    QImage m_beforeImage;
    QColor m_targetColor;   // colour sampled under the initial click
    QColor m_replaceColor;  // colour to paint in (primary)
};
