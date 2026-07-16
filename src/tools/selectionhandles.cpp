#include "selectionhandles.h"
#include "../core/selection.h"

#include <QPainter>
#include <algorithm>

namespace SelHandles {

double radius(const QRect &rect, double zoom) {
    const double screenRadius = 4.0;
    return std::min({screenRadius / std::max(zoom, 0.01),
                     rect.width() / 4.0,
                     rect.height() / 4.0});
}

// The eight anchors, in the same order as the Handle enum values below.
static QVector<QPointF> anchors(const QRect &rect) {
    return {
        rect.topLeft(),
        QPointF(rect.center().x(), rect.top()),
        rect.topRight(),
        QPointF(rect.right(), rect.center().y()),
        rect.bottomRight(),
        QPointF(rect.center().x(), rect.bottom()),
        rect.bottomLeft(),
        QPointF(rect.left(), rect.center().y())
    };
}

Handle at(const QRect &rect, const QPointF &canvasPos, double zoom) {
    if (rect.isEmpty()) return Handle::None;

    const double r = radius(rect, zoom);
    static const Handle order[] = {
        Handle::TopLeft, Handle::Top, Handle::TopRight, Handle::Right,
        Handle::BottomRight, Handle::Bottom, Handle::BottomLeft, Handle::Left
    };

    const QVector<QPointF> pts = anchors(rect);
    for (int i = 0; i < pts.size(); ++i) {
        const QRectF hit(pts[i].x() - r, pts[i].y() - r, r * 2.0, r * 2.0);
        if (hit.contains(canvasPos)) return order[i];
    }
    return Handle::None;
}

QRect resized(const QRect &rect, Handle handle, const QPointF &canvasPos) {
    QRect result = rect;
    const QPoint pos = canvasPos.toPoint();

    switch (handle) {
    case Handle::TopLeft:     result.setTopLeft(pos); break;
    case Handle::Top:         result.setTop(pos.y()); break;
    case Handle::TopRight:    result.setTopRight(pos); break;
    case Handle::Right:       result.setRight(pos.x()); break;
    case Handle::BottomRight: result.setBottomRight(pos); break;
    case Handle::Bottom:      result.setBottom(pos.y()); break;
    case Handle::BottomLeft:  result.setBottomLeft(pos); break;
    case Handle::Left:        result.setLeft(pos.x()); break;
    case Handle::None:        break;
    }

    QRect normalized = result.normalized();
    if (normalized.width() < 2) normalized.setWidth(2);
    if (normalized.height() < 2) normalized.setHeight(2);
    return normalized;
}

QVector<QRectF> rects(const QRect &rect, double zoom) {
    const qreal r = radius(rect, zoom);
    QVector<QRectF> out;
    for (const QPointF &p : anchors(rect))
        out.append(QRectF(p.x() - r, p.y() - r, r * 2.0, r * 2.0));
    return out;
}

void setScaledMask(Selection &sel, const QImage &originalMask,
                   const QRect &from, const QRect &to, bool smooth) {
    if (originalMask.isNull() || from.isEmpty() || to.isEmpty()) return;

    QImage newMask(originalMask.size(), QImage::Format_Grayscale8);
    newMask.fill(0);
    const QImage scaled = originalMask.copy(from)
                              .scaled(to.size(), Qt::IgnoreAspectRatio,
                                      smooth ? Qt::SmoothTransformation : Qt::FastTransformation)
                              .convertToFormat(QImage::Format_Grayscale8);
    QPainter p(&newMask);
    p.drawImage(to.topLeft(), scaled);
    p.end();
    sel.setMaskImage(newMask);
}

} // namespace SelHandles
