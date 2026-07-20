#include "vignetteeffect.h"
#include <cmath>

QImage VignetteEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    double cx = w / 2.0, cy = h / 2.0;
    // dx/dy below are normalised by cx/cy, so the corner distance is sqrt(2)
    // (the 1.414 further down) rather than the pixel radius.
    double radius = m_radius / 100.0;
    double strength = m_amount / 100.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double dx = (x - cx) / cx;
            double dy = (y - cy) / cy;
            double dist = std::sqrt(dx * dx + dy * dy);
            double vignette = 1.0;
            if (dist > radius) {
                double fade = (dist - radius) / (1.414 - radius);
                vignette = 1.0 - strength * fade * fade;
            }
            vignette = qBound(0.0, vignette, 1.0);
            QRgb p = img.pixel(x, y);
            dst[x] = qRgba(qBound(0, (int)(qRed(p) * vignette), 255),
                           qBound(0, (int)(qGreen(p) * vignette), 255),
                           qBound(0, (int)(qBlue(p) * vignette), 255),
                           qAlpha(p));
        }
    }
    return result;
}
