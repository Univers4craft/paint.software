#pragma once
#include "tool.h"
#include <QImage>

class FillTool : public Tool {
public:
    ToolType type() const override { return ToolType::Fill; }
    QString name() const override { return "Pot de peinture"; }
    QCursor cursor() const override;

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;

    // Flood mode, like Paint.NET's Paint Bucket. Local = contiguous region from
    // the click; Global = every matching pixel in the layer. Shift forces Global
    // for one click.
    bool isGlobal() const { return m_global; }
    void setGlobal(bool g) { m_global = g; }

private:
    void floodFill(QImage &image, const QPoint &pos, const QColor &fillColor, class Document *doc, bool global);
    bool m_global = false;
};
