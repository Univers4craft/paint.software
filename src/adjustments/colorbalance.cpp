#include "colorbalance.h"

QImage ColorBalance::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = qRed(line[x]) + m_cyanRed;
            int g = qGreen(line[x]) + m_magentaGreen;
            int b = qBlue(line[x]) + m_yellowBlue;
            line[x] = qRgba(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qAlpha(line[x]));
        }
    }
    return result;
}
