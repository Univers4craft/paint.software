#include "zoomtool.h"
#include "../canvas/canvaswidget.h"
#include <QMouseEvent>
#include <QCursor>
#include <QPixmap>
#include <QPainter>
#include <algorithm>

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
    Q_UNUSED(canvas);
    if (event->button() == Qt::LeftButton) {
        // A left press may become a zoom-to-rectangle drag; decide on release.
        m_dragging = true;
        m_startCanvas = canvasPos;
        m_currentCanvas = canvasPos;
    }
}

void ZoomTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_dragging) return;
    m_currentCanvas = canvasPos;
    canvas.update();
}

void ZoomTool::mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    const bool wasDragging = m_dragging;
    m_dragging = false;

    // A dragged rectangle (in widget pixels) big enough to mean "zoom here"
    // maximises that region, like Paint.NET. Anything smaller is a plain click.
    if (wasDragging && event->button() == Qt::LeftButton) {
        const QPointF a = canvas.canvasToWidget(m_startCanvas);
        const QPointF b = canvas.canvasToWidget(canvasPos);
        const QRectF box = QRectF(a, b).normalized();
        if (box.width() > 8 && box.height() > 8) {
            const QRectF rectCanvas = QRectF(m_startCanvas, canvasPos).normalized();
            const int rs = canvas.rulerSize();
            const double vw = canvas.width() - rs, vh = canvas.height() - rs;
            const double z = std::min(vw / std::max(1.0, rectCanvas.width()),
                                      vh / std::max(1.0, rectCanvas.height()));
            canvas.setZoom(z);
            // Centre the chosen region in the viewport.
            const QPointF centreCanvas = rectCanvas.center();
            const QPointF want(rs + vw / 2.0, rs + vh / 2.0);
            const QPointF haveNoPan = centreCanvas * canvas.zoom() + QPointF(rs, rs);
            canvas.setPan(want - haveNoPan);
            canvas.update();
            return;
        }
    }

    // Plain click: left zooms in, right zooms out, point under cursor fixed.
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        QPointF oldCanvas = canvas.widgetToCanvas(event->position());
        double factor = (event->button() == Qt::LeftButton) ? 1.5 : (1.0 / 1.5);
        canvas.setZoom(canvas.zoom() * factor);
        QPointF newWidget = canvas.canvasToWidget(oldCanvas);
        canvas.setPan(canvas.pan() + (event->position() - newWidget));
    }
}

void ZoomTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    if (!m_dragging) return;
    const QPointF a = canvas.canvasToWidget(m_startCanvas);
    const QPointF b = canvas.canvasToWidget(m_currentCanvas);
    const QRectF box = QRectF(a, b).normalized();
    if (box.width() < 2 && box.height() < 2) return;
    painter.setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
    painter.setBrush(QColor(0, 120, 215, 40));
    painter.drawRect(box);
}
