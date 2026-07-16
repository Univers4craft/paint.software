#pragma once
#include "tool.h"
#include <QImage>

class MoveTool : public Tool {
public:
    ToolType type() const override { return ToolType::Move; }
    QString name() const override { return "Déplacer les pixels sélectionnés"; }
    QCursor cursor() const override { return Qt::SizeAllCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    bool m_moving = false;
    QPointF m_startPos;
    QPoint m_originalOffset;

    // When a selection is active we move only the selected pixels (like Paint.NET's
    // "Move Selected Pixels"): lift them onto a floating layer, leave a hole behind,
    // and drag both the pixels and the marquee.
    bool m_movingSelection = false;
    QImage m_originalImage;   // layer pixels at press (for undo + recompositing)
    QImage m_floating;        // the selected pixels on transparent, document-sized
    QImage m_hole;            // the layer with the selected pixels erased
    QPoint m_lastDelta;       // last applied marquee translation (incremental)
};
