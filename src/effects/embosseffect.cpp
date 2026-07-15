#include "embosseffect.h"
#include <cmath>

QImage EmbossEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    // Neutral gray border so the unwritten 1px frame isn't uninitialized memory.
    QImage result(src.size(), QImage::Format_ARGB32);
    result.fill(qRgb(128, 128, 128));
    int w = src.width(), h = src.height();

    // Emboss convolution kernel based on angle
    double rad = m_angle * M_PI / 180.0;
    int dx = static_cast<int>(round(cos(rad)));
    int dy = static_cast<int>(round(sin(rad)));
    if (dx == 0 && dy == 0) dx = 1;

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            QRgb p1 = src.pixel(qBound(0, x - dx, w-1), qBound(0, y - dy, h-1));
            QRgb p2 = src.pixel(qBound(0, x + dx, w-1), qBound(0, y + dy, h-1));

            int r = qBound(0, qRed(p2) - qRed(p1) + 128, 255);
            int g = qBound(0, qGreen(p2) - qGreen(p1) + 128, 255);
            int b = qBound(0, qBlue(p2) - qBlue(p1) + 128, 255);
            result.setPixel(x, y, qRgba(r, g, b, qAlpha(src.pixel(x, y))));
        }
    }
    return result;
}
