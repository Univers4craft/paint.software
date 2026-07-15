#include "morphologyeffect.h"

QImage MorphologyEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    int r = m_radius;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int bestR, bestG, bestB;
            if (m_dilate) {
                bestR = 0; bestG = 0; bestB = 0;
            } else {
                bestR = 255; bestG = 255; bestB = 255;
            }
            for (int ky = -r; ky <= r; ++ky) {
                for (int kx = -r; kx <= r; ++kx) {
                    int nx = qBound(0, x + kx, w - 1);
                    int ny = qBound(0, y + ky, h - 1);
                    QRgb p = img.pixel(nx, ny);
                    if (m_dilate) {
                        bestR = qMax(bestR, qRed(p));
                        bestG = qMax(bestG, qGreen(p));
                        bestB = qMax(bestB, qBlue(p));
                    } else {
                        bestR = qMin(bestR, qRed(p));
                        bestG = qMin(bestG, qGreen(p));
                        bestB = qMin(bestB, qBlue(p));
                    }
                }
            }
            dst[x] = qRgba(bestR, bestG, bestB, qAlpha(img.pixel(x, y)));
        }
    }
    return result;
}
