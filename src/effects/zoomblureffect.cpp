#include "zoomblureffect.h"
#include <cmath>

QImage ZoomBlurEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    int cx = w / 2, cy = h / 2;
    int samples = qBound(4, m_amount * 2, 64);

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double dx = x - cx, dy = y - cy;
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0, count = 0;

            for (int s = 0; s < samples; ++s) {
                double t = 1.0 - (m_amount / 100.0) * s / samples;
                int sx = cx + static_cast<int>(dx * t);
                int sy = cy + static_cast<int>(dy * t);
                if (sx >= 0 && sx < w && sy >= 0 && sy < h) {
                    QRgb p = img.pixel(sx, sy);
                    rSum += qRed(p); gSum += qGreen(p); bSum += qBlue(p); aSum += qAlpha(p);
                    count++;
                }
            }
            if (count > 0)
                dst[x] = qRgba(rSum / count, gSum / count, bSum / count, aSum / count);
        }
    }
    return result;
}
