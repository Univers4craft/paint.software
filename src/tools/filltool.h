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

    // Sampling source (Paint.NET). true = Image (composite of visible layers)
    // decides which pixels to fill; false = Layer (the active layer). Either way
    // the paint is written to the active layer. Default Image, like Paint.NET.
    bool sampleImage() const { return m_sampleImage; }
    void setSampleImage(bool image) { m_sampleImage = image; }

private:
    // Fills the active layer (target). The region to fill is decided from
    // sampleSrc (the composite when Sampling=Image, or the layer itself when
    // Sampling=Layer); the chosen blend mode composites the fill onto target.
    void floodFill(QImage &target, const QImage &sampleSrc, const QPoint &pos,
                   const QColor &fillColor, class Document *doc, bool global);
    bool m_global = false;
    bool m_sampleImage = true;
};
