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

    QImage flat = doc->flattenVisible();
    QPoint pos = canvasPos.toPoint();
    if (pos.x() < 0 || pos.y() < 0 || pos.x() >= flat.width() || pos.y() >= flat.height()) return;

    QColor color = flat.pixelColor(pos);
    if (event->buttons() & Qt::LeftButton)
        doc->setPrimaryColor(color);
    else if (event->buttons() & Qt::RightButton)
        doc->setSecondaryColor(color);
}
