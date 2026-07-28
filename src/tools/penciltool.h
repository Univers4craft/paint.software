#pragma once
#include "tool.h"

class PencilTool : public Tool {
public:
    ToolType type() const override { return ToolType::Pencil; }
    QString name() const override { return "Crayon"; }
    QCursor cursor() const override { return Qt::CrossCursor; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

private:
    // Writes one pixel applying the current blend mode. Normal/Overwrite replace
    // the pixel outright (current behaviour); other modes composite the source
    // colour onto the destination via the shared blend-mode mapping.
    void plot(QImage &img, int x, int y, const QColor &color);

    bool m_drawing = false;
    QPoint m_lastPos;
    QImage m_beforeImage;
};
