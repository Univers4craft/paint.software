#include "motionblureffect.h"
#include <cmath>

QImage MotionBlurEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(src.size(), QImage::Format_ARGB32);
    int w = src.width(), h = src.height();

    double rad = m_angle * M_PI / 180.0;
    double dx = cos(rad), dy = sin(rad);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            int count = 0;

            for (int i = -m_distance / 2; i <= m_distance / 2; ++i) {
                int sx = qBound(0, x + static_cast<int>(i * dx), w - 1);
                int sy = qBound(0, y + static_cast<int>(i * dy), h - 1);
                QRgb p = src.pixel(sx, sy);
                rSum += qRed(p); gSum += qGreen(p); bSum += qBlue(p); aSum += qAlpha(p);
                ++count;
            }

            result.setPixel(x, y, qRgba(
                static_cast<int>(rSum/count),
                static_cast<int>(gSum/count),
                static_cast<int>(bSum/count),
                static_cast<int>(aSum/count)));
        }
    }
    return result;
}
