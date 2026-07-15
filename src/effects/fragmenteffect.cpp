#include "fragmenteffect.h"
#include <cmath>

QImage FragmentEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(img.size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    int w = img.width(), h = img.height();

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            for (int f = 0; f < m_fragments; ++f) {
                double angle = 2.0 * M_PI * f / m_fragments;
                int sx = x + static_cast<int>(m_distance * std::cos(angle));
                int sy = y + static_cast<int>(m_distance * std::sin(angle));
                sx = qBound(0, sx, w - 1);
                sy = qBound(0, sy, h - 1);
                QRgb p = img.pixel(sx, sy);
                rSum += qRed(p); gSum += qGreen(p); bSum += qBlue(p); aSum += qAlpha(p);
            }
            dst[x] = qRgba(rSum / m_fragments, gSum / m_fragments, bSum / m_fragments, aSum / m_fragments);
        }
    }
    return result;
}
