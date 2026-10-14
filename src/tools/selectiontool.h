#pragma once
#include "tool.h"
#include <QRect>

enum class SelectionShape { Rectangle, Ellipse };

// Matches paint.NET's "Selection" mode dropdown.
enum class SelectionConstraint { Normal, FixedRatio, FixedSize };

class SelectionTool : public Tool {
public:
    SelectionTool(SelectionShape shape = SelectionShape::Rectangle);

    ToolType type() const override { return m_shape == SelectionShape::Rectangle ? ToolType::RectSelection : ToolType::EllipseSelection; }
    QString name() const override { return m_shape == SelectionShape::Rectangle ? "Sélection rectangle" : "Sélection ellipse"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

    SelectionShape shape() const { return m_shape; }
    void setShape(SelectionShape shape) { m_shape = shape; }

    SelectionConstraint constraint() const { return m_constraint; }
    void setConstraint(SelectionConstraint c) { m_constraint = c; }

    // W and H values for FixedRatio and FixedSize modes.
    double constraintW() const { return m_cw; }
    double constraintH() const { return m_ch; }
    void setConstraintW(double v) { m_cw = qMax(1.0, v); }
    void setConstraintH(double v) { m_ch = qMax(1.0, v); }

private:
    QPointF applyConstraint(const QPointF &start, const QPointF &current) const;

    SelectionShape m_shape;
    SelectionConstraint m_constraint = SelectionConstraint::Normal;
    double m_cw = 1.0;
    double m_ch = 1.0;
    bool m_selecting = false;
    Qt::MouseButton m_button = Qt::LeftButton;
    QPointF m_startPos;
    QPointF m_currentPos;
};
