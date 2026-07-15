#include "bulgeeffect.h"
#include <cmath>

QImage BulgeEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(img.size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    int w = img.width(), h = img.height();
    double cx = w / 2.0, cy = h / 2.0;
    double maxR = std::min(cx, cy);
    double bulge = m_amount / 100.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double dx = (x - cx) / maxR;
            double dy = (y - cy) / maxR;
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 1e-9) {
                // Exact centre: no displacement (avoids 0/0 → NaN → UB cast).
                dst[x] = img.pixel(x, y);
            } else if (dist < 1.0) {
                double newDist = std::pow(dist, 1.0 + bulge);
                double sx = cx + dx / dist * newDist * maxR;
                double sy = cy + dy / dist * newDist * maxR;
                int ix = qBound(0, (int)sx, w - 1);
                int iy = qBound(0, (int)sy, h - 1);
                dst[x] = img.pixel(ix, iy);
            } else {
                dst[x] = img.pixel(x, y);
            }
        }
    }
    return result;
}
