#include "twisteffect.h"
#include <cmath>

QImage TwistEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(img.size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    int w = img.width(), h = img.height();
    double cx = w / 2.0, cy = h / 2.0;
    double maxR = std::sqrt(cx * cx + cy * cy);
    double twistAngle = m_amount * M_PI / 180.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double dx = x - cx, dy = y - cy;
            double dist = std::sqrt(dx * dx + dy * dy);
            double angle = std::atan2(dy, dx);
            double twist = twistAngle * (1.0 - dist / maxR);
            angle += twist;
            int sx = qBound(0, (int)(cx + dist * std::cos(angle)), w - 1);
            int sy = qBound(0, (int)(cy + dist * std::sin(angle)), h - 1);
            dst[x] = img.pixel(sx, sy);
        }
    }
    return result;
}
