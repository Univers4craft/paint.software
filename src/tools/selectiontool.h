#pragma once
#include "tool.h"
#include <QRect>

enum class SelectionShape { Rectangle, Ellipse };

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

private:
    SelectionShape m_shape;
    bool m_selecting = false;
    Qt::MouseButton m_button = Qt::LeftButton;   // which button started the drag (combine mode)
    QPointF m_startPos;
    QPointF m_currentPos;
};
