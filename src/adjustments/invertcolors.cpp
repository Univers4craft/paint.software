#include "invertcolors.h"

QImage InvertColors::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            line[x] = qRgba(255 - qRed(line[x]), 255 - qGreen(line[x]), 255 - qBlue(line[x]), qAlpha(line[x]));
        }
    }
    return result;
}
