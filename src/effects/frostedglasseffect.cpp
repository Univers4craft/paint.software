#include "frostedglasseffect.h"
#include <cstdlib>

QImage FrostedGlassEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int ox = x + (rand() % (2 * m_amount + 1)) - m_amount;
            int oy = y + (rand() % (2 * m_amount + 1)) - m_amount;
            ox = qBound(0, ox, w - 1);
            oy = qBound(0, oy, h - 1);
            dst[x] = img.pixel(ox, oy);
        }
    }
    return result;
}
