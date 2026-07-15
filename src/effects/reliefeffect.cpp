#include "reliefeffect.h"
#include <cmath>

QImage ReliefEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    double rad = m_angle * M_PI / 180.0;
    // round, not truncate: at 45° cos/sin are 0.707 and would truncate to 0,
    // making every output pixel a flat 128 gray.
    int dx = static_cast<int>(std::lround(std::cos(rad)));
    int dy = static_cast<int>(std::lround(std::sin(rad)));
    if (dx == 0 && dy == 0) dx = 1;

    for (int y = 1; y < h - 1; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 1; x < w - 1; ++x) {
            QRgb p1 = img.pixel(qBound(0, x - dx, w - 1), qBound(0, y - dy, h - 1));
            QRgb p2 = img.pixel(qBound(0, x + dx, w - 1), qBound(0, y + dy, h - 1));
            int r = qBound(0, (qRed(p1) - qRed(p2)) + 128, 255);
            int g = qBound(0, (qGreen(p1) - qGreen(p2)) + 128, 255);
            int b = qBound(0, (qBlue(p1) - qBlue(p2)) + 128, 255);
            dst[x] = qRgba(r, g, b, qAlpha(img.pixel(x, y)));
        }
    }
    return result;
}
