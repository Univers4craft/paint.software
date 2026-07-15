#include "outlineeffect.h"
#include <cmath>

QImage OutlineEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(img.size(), QImage::Format_ARGB32);
    int w = img.width(), h = img.height();
    int r = m_thickness;
    double intensityF = m_intensity / 100.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            // Morphological gradient: dilate - erode
            int maxR = 0, maxG = 0, maxB = 0;
            int minR = 255, minG = 255, minB = 255;
            for (int ky = -r; ky <= r; ++ky) {
                for (int kx = -r; kx <= r; ++kx) {
                    if (kx * kx + ky * ky > r * r) continue;
                    int nx = qBound(0, x + kx, w - 1);
                    int ny = qBound(0, y + ky, h - 1);
                    QRgb p = img.pixel(nx, ny);
                    maxR = qMax(maxR, qRed(p)); minR = qMin(minR, qRed(p));
                    maxG = qMax(maxG, qGreen(p)); minG = qMin(minG, qGreen(p));
                    maxB = qMax(maxB, qBlue(p)); minB = qMin(minB, qBlue(p));
                }
            }
            int rr = qBound(0, (int)((maxR - minR) * intensityF), 255);
            int gg = qBound(0, (int)((maxG - minG) * intensityF), 255);
            int bb = qBound(0, (int)((maxB - minB) * intensityF), 255);
            dst[x] = qRgba(rr, gg, bb, qAlpha(img.pixel(x, y)));
        }
    }
    return result;
}
