#include "reducenoiseeffect.h"
#include <cmath>

QImage ReduceNoiseEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    double sigma = m_strength / 10.0;
    double factor = -0.5 / (sigma * sigma);

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb center = img.pixel(x, y);
            double rSum = 0, gSum = 0, bSum = 0, wSum = 0;

            for (int ky = -m_radius; ky <= m_radius; ++ky) {
                for (int kx = -m_radius; kx <= m_radius; ++kx) {
                    int nx = qBound(0, x + kx, w - 1);
                    int ny = qBound(0, y + ky, h - 1);
                    QRgb p = img.pixel(nx, ny);
                    int diff = std::abs(qRed(p) - qRed(center)) +
                               std::abs(qGreen(p) - qGreen(center)) +
                               std::abs(qBlue(p) - qBlue(center));
                    double spatialDist = kx * kx + ky * ky;
                    double weight = std::exp(factor * spatialDist) * std::exp(-diff * diff / (2.0 * m_strength * m_strength));
                    rSum += qRed(p) * weight;
                    gSum += qGreen(p) * weight;
                    bSum += qBlue(p) * weight;
                    wSum += weight;
                }
            }
            if (wSum > 0)
                dst[x] = qRgba(qBound(0, (int)(rSum / wSum), 255), qBound(0, (int)(gSum / wSum), 255),
                                qBound(0, (int)(bSum / wSum), 255), qAlpha(center));
        }
    }
    return result;
}
