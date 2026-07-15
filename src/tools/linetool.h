#pragma once
#include "tool.h"
#include <QImage>

class LineTool : public Tool {
public:
    ToolType type() const override { return ToolType::Line; }
    QString name() const override { return "Line / Curve"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

    enum class LineStyle { Solid, Arrow, DashedLine };
    LineStyle lineStyle() const { return m_lineStyle; }
    void setLineStyle(LineStyle s) { m_lineStyle = s; }

private:
    bool m_drawing = false;
    QPointF m_startPos, m_currentPos;
    QImage m_beforeImage;
    LineStyle m_lineStyle = LineStyle::Solid;
};
