#include "redeyeremoveeffect.h"
#include <cmath>

QImage RedEyeRemoveEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    double tol = m_tolerance / 100.0;
    double sat = m_saturation / 100.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb p = img.pixel(x, y);
            int r = qRed(p), g = qGreen(p), b = qBlue(p);
            // Check if pixel is "red" (high red relative to green and blue)
            double redIntensity = (r > 0) ? (double)r / qMax(1, qMax(g, b)) : 0;
            if (redIntensity > (2.0 - tol) && r > 80) {
                // Desaturate the red channel
                int gray = (r + g + b) / 3;
                int newR = (int)(gray * sat + r * (1 - sat));
                dst[x] = qRgba(qBound(0, newR, 255), g, b, qAlpha(p));
            }
        }
    }
    return result;
}
