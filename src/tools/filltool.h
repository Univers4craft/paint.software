#pragma once
#include "tool.h"
#include <QImage>

class FillTool : public Tool {
public:
    ToolType type() const override { return ToolType::Fill; }
    QString name() const override { return "Paint Bucket"; }
    QCursor cursor() const override;

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    void floodFill(QImage &image, const QPoint &pos, const QColor &fillColor, class Document *doc);
};
