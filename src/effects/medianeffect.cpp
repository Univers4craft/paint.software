#include "medianeffect.h"
#include <algorithm>
#include <vector>

QImage MedianEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    int r = m_radius;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            std::vector<int> reds, greens, blues;
            for (int ky = -r; ky <= r; ++ky) {
                for (int kx = -r; kx <= r; ++kx) {
                    int nx = qBound(0, x + kx, w - 1);
                    int ny = qBound(0, y + ky, h - 1);
                    QRgb p = img.pixel(nx, ny);
                    reds.push_back(qRed(p));
                    greens.push_back(qGreen(p));
                    blues.push_back(qBlue(p));
                }
            }
            std::sort(reds.begin(), reds.end());
            std::sort(greens.begin(), greens.end());
            std::sort(blues.begin(), blues.end());
            int mid = reds.size() / 2;
            dst[x] = qRgba(reds[mid], greens[mid], blues[mid], qAlpha(img.pixel(x, y)));
        }
    }
    return result;
}
