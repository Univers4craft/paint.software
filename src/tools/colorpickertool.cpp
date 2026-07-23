#include "colorpickertool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"

void ColorPickerTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    pickColor(canvasPos, event, canvas);
}

void ColorPickerTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
        pickColor(canvasPos, event, canvas);
}

void ColorPickerTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}

void ColorPickerTool::pickColor(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    auto *doc = canvas.document();
    if (!doc) return;

    // Paint.NET samples the active LAYER by default; hold Ctrl to sample the
    // composite (Image) instead.
    auto *layer = doc->activeLayer();
    const bool sampleImage = (event->modifiers() & Qt::ControlModifier) || !layer;
    QImage src = sampleImage ? doc->flattenVisible() : layer->image();
    QPoint pos = toPixelPos(canvasPos);
    if (pos.x() < 0 || pos.y() < 0 || pos.x() >= src.width() || pos.y() >= src.height()) return;

    QColor color = src.pixelColor(pos);
    if (event->buttons() & Qt::LeftButton)
        doc->setPrimaryColor(color);
    else if (event->buttons() & Qt::RightButton)
        doc->setSecondaryColor(color);
}
