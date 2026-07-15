#include "zoomtool.h"
#include "../canvas/canvaswidget.h"
#include <QMouseEvent>
#include <QCursor>
#include <QPixmap>
#include <QPainter>

QCursor ZoomTool::cursor() const {
    // Magnifier cursor: draw a 24x24 magnifier icon
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(3, 3, 13, 13);
    p.drawLine(14, 14, 20, 20);
    p.setPen(QPen(QColor(80, 80, 80), 1.2));
    p.drawLine(8, 9, 11, 9);
    p.drawLine(9, 6, 9, 12);
    p.end();
    return QCursor(pm, 9, 9);
}

void ZoomTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    Q_UNUSED(canvasPos);
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        // Keep the point under the cursor fixed while zooming.
        QPointF oldCanvas = canvas.widgetToCanvas(event->position());
        double factor = (event->button() == Qt::LeftButton) ? 1.5 : (1.0 / 1.5);
        canvas.setZoom(canvas.zoom() * factor);
        QPointF newWidget = canvas.canvasToWidget(oldCanvas);
        canvas.setPan(canvas.pan() + (event->position() - newWidget));
    }
}
