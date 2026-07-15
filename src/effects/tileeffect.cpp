#include "tileeffect.h"
#include <cmath>

QImage TileEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(img.size(), QImage::Format_ARGB32);
    int w = img.width(), h = img.height();
    double angle = m_rotation * M_PI / 180.0;
    double cosA = std::cos(angle), sinA = std::sin(angle);

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            // Rotate, tile, rotate back
            double rx = x * cosA + y * sinA;
            double ry = -x * sinA + y * cosA;
            // Tile using modulo
            int tx = ((int)rx % m_tileSize + m_tileSize) % m_tileSize;
            int ty = ((int)ry % m_tileSize + m_tileSize) % m_tileSize;
            // Mirror for reflection
            if (((int)(rx / m_tileSize)) % 2 != 0) tx = m_tileSize - 1 - tx;
            if (((int)(ry / m_tileSize)) % 2 != 0) ty = m_tileSize - 1 - ty;
            // Rotate back
            double fx = tx * cosA - ty * sinA;
            double fy = tx * sinA + ty * cosA;
            int sx = qBound(0, (int)fx, w - 1);
            int sy = qBound(0, (int)fy, h - 1);
            dst[x] = img.pixel(sx, sy);
        }
    }
    return result;
}
