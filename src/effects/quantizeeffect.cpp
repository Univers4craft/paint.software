#include "quantizeeffect.h"
#include <cmath>

QImage QuantizeEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    // Uniform quantization: round each channel to nearest level
    int levels = qBound(2, m_colors, 256);
    double step = 255.0 / (levels - 1);

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb p = img.pixel(x, y);
            int r = qBound(0, (int)(std::round(qRed(p) / step) * step), 255);
            int g = qBound(0, (int)(std::round(qGreen(p) / step) * step), 255);
            int b = qBound(0, (int)(std::round(qBlue(p) / step) * step), 255);
            dst[x] = qRgba(r, g, b, qAlpha(p));
        }
    }
    return result;
}
