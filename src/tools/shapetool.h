#pragma once
#include "tool.h"
#include <QImage>

enum class ShapeType { Rectangle, Ellipse, RoundedRectangle, Triangle, Diamond, Pentagon, Hexagon, Star };
enum class ShapeFill { Outline, Filled, Both };

class ShapeTool : public Tool {
public:
    ToolType type() const override { return ToolType::Shape; }
    QString name() const override { return "Shapes"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

    ShapeType shapeType() const { return m_shapeType; }
    void setShapeType(ShapeType t) { m_shapeType = t; }

    ShapeFill shapeFill() const { return m_shapeFill; }
    void setShapeFill(ShapeFill f) { m_shapeFill = f; }

private:
    void drawShape(QPainter &painter, const QRectF &rect);

    ShapeType m_shapeType = ShapeType::Rectangle;
    ShapeFill m_shapeFill = ShapeFill::Outline;
    bool m_drawing = false;
    QPointF m_startPos, m_currentPos;
    QImage m_beforeImage;
    bool m_useRight = false;
};
