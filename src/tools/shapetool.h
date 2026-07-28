#pragma once
#include "tool.h"
#include <QImage>
#include <QRectF>
#include <QPointF>
#include <QTransform>

// The full shape catalogue, in the order shown by the tool-options dropdown.
// Values are generic geometry only — no proprietary artwork is reproduced.
enum class ShapeType {
    Rectangle, RoundedRectangle, Ellipse, Diamond, Triangle,
    Trapezoid, Parallelogram, RightTriangle,
    Pentagon, Hexagon, Heptagon, Octagon,
    Star3, Star4, Star, Star6,          // Star == the classic 5-point star
    Arrow, Chevron,
    SpeechBalloon, Cloud,
    Heart, Lightning, Cross, Check
};
enum class ShapeFill { Outline, Filled, Both };

// Paint.NET's Shapes tool: drag to lay out the bounding box, then the shape stays
// live and editable — move it, resize it via 8 nubs, rotate it with the handle —
// until you commit with Enter (or by clicking away / switching tools). Nothing is
// rasterised into the layer until commit, exactly like the Line / Curve tool.
class ShapeTool : public Tool {
public:
    ToolType type() const override { return ToolType::Shape; }
    QString name() const override { return "Formes"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void keyPressEvent(QKeyEvent *event, CanvasWidget &canvas) override;
    bool wantsCommitKey(int key) const override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;
    void deactivate(CanvasWidget &canvas) override;

    ShapeType shapeType() const { return m_shapeType; }
    void setShapeType(ShapeType t) { m_shapeType = t; }

    ShapeFill shapeFill() const { return m_shapeFill; }
    void setShapeFill(ShapeFill f) { m_shapeFill = f; }

    int cornerSize() const { return m_cornerSize; }
    void setCornerSize(int s) { m_cornerSize = qBound(0, s, 200); }

private:
    enum class State { Idle, Dragging, Editing };
    // The eight resize nubs, the rotation handle, plus "inside the box" (move) and
    // "nothing" (a click away that commits).
    enum class Handle { None, TopLeft, Top, TopRight, Right,
                        BottomRight, Bottom, BottomLeft, Left, Rotate, Move };

    // Pure geometry: builds the shape's path inside an axis-aligned rect. Rotation
    // is applied separately by the caller via a QTransform about the rect centre.
    QPainterPath shapePath(const QRectF &rect) const;

    // Applies the Shift (1:1 constrain) and Alt (draw-from-centre) modifiers to
    // m_startPos/m_currentPos to build the initial drag rectangle.
    QRectF effectiveRect() const;

    // Rotation about a rect's centre, in canvas space.
    static QTransform rotationTransform(const QRectF &rect, double angleDeg);
    // Local (un-rotated) canvas position of a resize handle on the given rect.
    static QPointF handleLocal(Handle h, const QRectF &rect);
    // Which handle (if any) a widget-space point is over, for the current edit rect.
    Handle handleAt(const QPointF &widgetPos, const CanvasWidget &canvas) const;
    // Widget-space position of a handle for the current edit rect (incl. rotation).
    QPointF handleWidget(Handle h, const CanvasWidget &canvas) const;

    void resizeTo(Handle h, const QPointF &canvasPos, Qt::KeyboardModifiers mods);
    void paintShape(QPainter &painter, const QColor &primary, const QColor &secondary,
                    const QRectF &rect) const;
    void commit(CanvasWidget &canvas);

    ShapeType m_shapeType = ShapeType::Rectangle;
    ShapeFill m_shapeFill = ShapeFill::Outline;
    State m_state = State::Idle;

    // Initial drag (Dragging state).
    QPointF m_startPos, m_currentPos;
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;

    // The live shape (Editing state): an axis-aligned bounding rect + rotation.
    QRectF m_rect;
    double m_angle = 0.0;          // degrees, clockwise, about the rect centre
    Handle m_activeHandle = Handle::None;
    QPointF m_pressCanvas;         // mouse-down position, for moving
    QRectF m_rectAtPress;          // rect snapshot at mouse-down, for moving

    QImage m_beforeImage;
    bool m_useRight = false;
    int m_cornerSize = 10;         // rounded-rectangle corner radius (0..200)
};
