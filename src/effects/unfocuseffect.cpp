#include "unfocuseffect.h"

QImage UnfocusEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    int r = m_radius;

    // Box blur (fast unfocus)
    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0, count = 0;
            for (int ky = -r; ky <= r; ++ky) {
                for (int kx = -r; kx <= r; ++kx) {
                    int nx = qBound(0, x + kx, w - 1);
                    int ny = qBound(0, y + ky, h - 1);
                    QRgb p = img.pixel(nx, ny);
                    rSum += qRed(p); gSum += qGreen(p); bSum += qBlue(p); aSum += qAlpha(p);
                    count++;
                }
            }
            dst[x] = qRgba(rSum / count, gSum / count, bSum / count, aSum / count);
        }
    }
    return result;
}
