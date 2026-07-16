#pragma once
#include "tool.h"
#include "selectionhandles.h"
#include <QImage>
#include <QRect>

class MoveTool : public Tool {
public:
    ToolType type() const override { return ToolType::Move; }
    QString name() const override { return "Déplacer les pixels sélectionnés"; }
    QCursor cursor() const override { return Qt::SizeAllCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

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

    // Dragging a handle rescales the artwork itself, not just the marquee: the
    // selected pixels are stretched or squashed onto the new bounds.
    bool m_resizing = false;
    SelHandles::Handle m_handle = SelHandles::Handle::None;
    QRect m_originalRect;     // selection bounds at press
    QRect m_previewRect;      // bounds being dragged to
    QImage m_floatingCrop;    // the lifted pixels, cropped to m_originalRect
    QImage m_originalMask;    // selection mask at press, rescaled alongside
};
