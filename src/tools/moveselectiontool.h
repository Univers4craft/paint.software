#pragma once
#include "tool.h"
#include "selectionhandles.h"
#include <QPoint>
#include <QImage>
#include <QRect>

// Moves the current selection marquee without moving pixel content. To stretch
// the artwork itself, use MoveTool ("Move Selected Pixels").
class MoveSelectionTool : public Tool {
public:
    ToolType type() const override { return ToolType::MoveSelection; }
    QString name() const override { return "Déplacer la sélection"; }
    QCursor cursor() const override { return Qt::SizeAllCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

private:
    bool m_moving = false;
    bool m_resizing = false;
    bool m_rotating = false;
    SelHandles::Handle m_activeHandle = SelHandles::Handle::None;
    QPointF m_startPos;
    QPointF m_lastPos;
    QRect m_originalSelectionRect;
    QRect m_previewSelectionRect;
    QImage m_originalSelectionMask;

    // Rotation of the marquee about the box centre (Paint.NET): drag just outside
    // the box, or with the right button. Shift snaps to 15°.
    QPointF m_rotCenter;
    double m_rotStartAngle = 0.0;
    double m_angle = 0.0;
};
