#include "sharpeneffect.h"

QImage SharpenEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = src.copy();  // start from src so the 1px border isn't left uninitialized
    int w = src.width(), h = src.height();
    double factor = m_amount / 50.0;

    // Unsharp mask kernel
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            QRgb center = src.pixel(x, y);
            int rSum = 0, gSum = 0, bSum = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    QRgb p = src.pixel(x + dx, y + dy);
                    rSum += qRed(p); gSum += qGreen(p); bSum += qBlue(p);
                }
            }
            int r = qRed(center) + static_cast<int>((qRed(center) - rSum / 8.0) * factor);
            int g = qGreen(center) + static_cast<int>((qGreen(center) - gSum / 8.0) * factor);
            int b = qBlue(center) + static_cast<int>((qBlue(center) - bSum / 8.0) * factor);
            result.setPixel(x, y, qRgba(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qAlpha(center)));
        }
    }
    return result;
}
