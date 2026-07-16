#pragma once
#include "tool.h"
#include <QPoint>
#include <QImage>
#include <QRect>

// Moves the current selection marquee without moving pixel content
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
    enum class ResizeHandle {
        None,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left
    };

    ResizeHandle handleAt(const QRect &rect, const QPointF &canvasPos, double zoom) const;
    QRect resizedRect(const QRect &rect, ResizeHandle handle, const QPointF &canvasPos) const;
    QRect normalizedWithMinimumSize(const QRect &rect) const;
    QVector<QRectF> handleRects(const QRect &rect, double zoom) const;
    double handleRadius(const QRect &rect, double zoom) const;

    bool m_moving = false;
    bool m_resizing = false;
    ResizeHandle m_activeHandle = ResizeHandle::None;
    QPointF m_startPos;
    QPointF m_lastPos;
    QRect m_originalSelectionRect;
    QRect m_previewSelectionRect;
    QImage m_originalSelectionMask;
};
