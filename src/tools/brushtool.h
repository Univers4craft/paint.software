#pragma once
#include "tool.h"
#include <QImage>
#include <QVector>

class BrushTool : public Tool {
public:
    ToolType type() const override { return ToolType::Brush; }
    QString name() const override { return "Paintbrush"; }
    QCursor cursor() const override { return Qt::CrossCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

private:
    void drawBrushStroke(const QPointF &from, const QPointF &to, const QColor &color);
    void drawBrushDab(const QPointF &pos, const QColor &color);
    void compositeStroke(class Document *doc, class Layer *layer, const QColor &color);
    void compositeOverwrite(class Document *doc, class Layer *layer, const QColor &color);
    QPainter::CompositionMode brushCompositionMode() const;

    bool m_drawing = false;
    QPointF m_lastPos;
    QImage m_beforeImage;   // layer pixels at stroke start
    QImage m_strokeBuffer;  // accumulates the current stroke's coverage
    QColor m_strokeColor;
    QPointF m_currentPos;
};
