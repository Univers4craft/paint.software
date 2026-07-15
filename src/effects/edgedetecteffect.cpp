#include "edgedetecteffect.h"
#include <cmath>

QImage EdgeDetectEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(src.size(), QImage::Format_ARGB32);
    result.fill(Qt::black);
    int w = src.width(), h = src.height();

    // Sobel edge detection
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            auto gray = [&](int px, int py) {
                QRgb p = src.pixel(px, py);
                return 0.299 * qRed(p) + 0.587 * qGreen(p) + 0.114 * qBlue(p);
            };

            double gx = -gray(x-1,y-1) + gray(x+1,y-1) - 2*gray(x-1,y) + 2*gray(x+1,y) - gray(x-1,y+1) + gray(x+1,y+1);
            double gy = -gray(x-1,y-1) - 2*gray(x,y-1) - gray(x+1,y-1) + gray(x-1,y+1) + 2*gray(x,y+1) + gray(x+1,y+1);
            int edge = qBound(0, static_cast<int>(std::sqrt(gx*gx + gy*gy)), 255);

            result.setPixel(x, y, qRgba(edge, edge, edge, qAlpha(src.pixel(x, y))));
        }
    }
    return result;
}
