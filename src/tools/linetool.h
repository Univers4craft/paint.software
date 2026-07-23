#pragma once
#include "tool.h"
#include <QImage>
#include <QColor>
#include <array>

// Paint.NET's Line / Curve tool: drag to lay down a straight segment, then bend
// it into a cubic Bézier by dragging the four control nubs that appear, and
// commit with Enter (or by clicking away / switching tools).
class LineTool : public Tool {
public:
    ToolType type() const override { return ToolType::Line; }
    QString name() const override { return "Ligne / Courbe"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void keyPressEvent(QKeyEvent *event, CanvasWidget &canvas) override;
    bool wantsCommitKey(int key) const override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;
    void deactivate(CanvasWidget &canvas) override;

    enum class LineStyle { Solid, Arrow, DashedLine };
    LineStyle lineStyle() const { return m_lineStyle; }
    void setLineStyle(LineStyle s) { m_lineStyle = s; }

private:
    enum class State { Idle, Dragging, Editing };

    // Lays the four control points out as a straight line from a to b: the two
    // inner points sit at 1/3 and 2/3, so the curve starts perfectly straight.
    void setStraight(const QPointF &a, const QPointF &b);
    // Index of a node within grab distance of a widget-space point, or -1.
    int nodeAt(const QPointF &widgetPos, const CanvasWidget &canvas) const;
    QPainterPath curvePath() const;
    void paintCurve(QPainter &painter, const QColor &color, bool antialiased) const;
    void commit(CanvasWidget &canvas);

    State m_state = State::Idle;
    std::array<QPointF, 4> m_pts;   // cubic Bézier: P0, P1, P2, P3 (canvas coords)
    int m_activeNode = -1;
    QColor m_color;
    QImage m_beforeImage;
    LineStyle m_lineStyle = LineStyle::Solid;
};
