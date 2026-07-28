#pragma once
#include "tool.h"
#include <QImage>

enum class ShapeType { Rectangle, Ellipse, RoundedRectangle, Triangle, Diamond, Pentagon, Hexagon, Star };
enum class ShapeFill { Outline, Filled, Both };

class ShapeTool : public Tool {
public:
    ToolType type() const override { return ToolType::Shape; }
    QString name() const override { return "Formes"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

    ShapeType shapeType() const { return m_shapeType; }
    void setShapeType(ShapeType t) { m_shapeType = t; }

    ShapeFill shapeFill() const { return m_shapeFill; }
    void setShapeFill(ShapeFill f) { m_shapeFill = f; }

    int cornerSize() const { return m_cornerSize; }
    void setCornerSize(int s) { m_cornerSize = qBound(0, s, 200); }

private:
    void drawShape(QPainter &painter, const QRectF &rect);
    // Applies the Shift (1:1 constrain) and Alt (draw-from-centre) modifiers to
    // m_startPos/m_currentPos, so the live preview and the commit share one rect.
    QRectF effectiveRect() const;

    ShapeType m_shapeType = ShapeType::Rectangle;
    ShapeFill m_shapeFill = ShapeFill::Outline;
    bool m_drawing = false;
    QPointF m_startPos, m_currentPos;
    QImage m_beforeImage;
    bool m_useRight = false;
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;   // captured during the drag
    int m_cornerSize = 10;   // rounded-rectangle corner radius (0..200)
};
