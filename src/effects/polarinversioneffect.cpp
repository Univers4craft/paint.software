#include "polarinversioneffect.h"
#include <cmath>

QImage PolarInversionEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(img.size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    int w = img.width(), h = img.height();
    double cx = w / 2.0, cy = h / 2.0;
    double maxR = std::sqrt(cx * cx + cy * cy);
    double t = m_amount / 100.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double dx = x - cx, dy = y - cy;
            double r = std::sqrt(dx * dx + dy * dy);
            double theta = std::atan2(dy, dx);
            // Inversion: swap r and theta mapping
            double newX = theta / M_PI * cx;
            double newY = r / maxR * h;
            // Blend between original and inverted
            double sx = x * (1 - t) + newX * t;
            double sy = y * (1 - t) + newY * t;
            int ix = qBound(0, (int)sx, w - 1);
            int iy = qBound(0, (int)sy, h - 1);
            dst[x] = img.pixel(ix, iy);
        }
    }
    return result;
}
