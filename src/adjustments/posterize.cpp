#include "posterize.h"

QImage Posterize::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();
    double step = 255.0 / (m_levels - 1);

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = static_cast<int>(qRound(qRed(line[x]) / step) * step);
            int g = static_cast<int>(qRound(qGreen(line[x]) / step) * step);
            int b = static_cast<int>(qRound(qBlue(line[x]) / step) * step);
            line[x] = qRgba(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qAlpha(line[x]));
        }
    }
    return result;
}
