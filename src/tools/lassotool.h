#pragma once
#include "tool.h"
#include <QPolygonF>
#include <QImage>

class LassoTool : public Tool {
public:
    ToolType type() const override { return ToolType::LassoSelection; }
    QString name() const override { return "Lasso"; }
    QCursor cursor() const override { return Qt::CrossCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

private:
    bool m_selecting = false;
    QPolygonF m_lasso;
};
