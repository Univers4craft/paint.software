#include "dentseffect.h"
#include <cmath>

QImage DentsEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(img.size(), QImage::Format_ARGB32);
    int w = img.width(), h = img.height();
    double freq = m_scale / 10.0;
    double amp = m_refraction;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double dx = std::sin(y * freq * 0.05) * amp * 0.3 + std::sin(y * freq * 0.02 + x * freq * 0.01) * amp * 0.15;
            double dy = std::cos(x * freq * 0.05) * amp * 0.3 + std::cos(x * freq * 0.02 + y * freq * 0.01) * amp * 0.15;
            int sx = qBound(0, x + (int)dx, w - 1);
            int sy = qBound(0, y + (int)dy, h - 1);
            dst[x] = img.pixel(sx, sy);
        }
    }
    return result;
}
