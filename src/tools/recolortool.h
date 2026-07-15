#pragma once
#include "tool.h"

class RecolorTool : public Tool {
public:
    ToolType type() const override { return ToolType::Recolor; }
    QString name() const override { return "Recolor"; }
    QCursor cursor() const override { return Qt::CrossCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    void recolorAt(const QPointF &pos, CanvasWidget &canvas);
    bool m_drawing = false;
    QImage m_beforeImage;
    QColor m_targetColor;   // colour sampled under the initial click
    QColor m_replaceColor;  // colour to paint in (primary)
};
