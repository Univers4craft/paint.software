#include "radialblureffect.h"
#include <cmath>

QImage RadialBlurEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    int cx = m_center.isNull() ? w / 2 : m_center.x();
    int cy = m_center.isNull() ? h / 2 : m_center.y();
    double angleRad = m_angle * M_PI / 180.0;
    int samples = qBound(2, m_angle * 3, 64);

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double dx = x - cx, dy = y - cy;
            double dist = std::sqrt(dx * dx + dy * dy);
            double baseAngle = std::atan2(dy, dx);
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0, count = 0;

            for (int s = 0; s < samples; ++s) {
                double a = baseAngle + angleRad * ((double)s / samples - 0.5);
                int sx = cx + static_cast<int>(dist * std::cos(a));
                int sy = cy + static_cast<int>(dist * std::sin(a));
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
