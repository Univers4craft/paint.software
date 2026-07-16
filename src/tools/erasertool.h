#pragma once
#include "tool.h"
#include <QImage>

class EraserTool : public Tool {
public:
    ToolType type() const override { return ToolType::Eraser; }
    QString name() const override { return "Gomme"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

private:
    void eraseStroke(const QPointF &from, const QPointF &to);
    void eraseDab(const QPointF &pos);
    void compositeErase(class Document *doc, class Layer *layer);

    bool m_drawing = false;
    bool m_toSecondary = false;   // right-click paints the secondary colour instead of erasing
    QPointF m_lastPos;
    QPointF m_currentPos;
    QImage m_beforeImage;
    QImage m_strokeBuffer;
};
