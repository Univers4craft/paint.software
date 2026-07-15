#include "invertalpha.h"

QImage InvertAlpha::apply(const QImage &input) {
    QImage r = input.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < r.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(r.scanLine(y));
        for (int x = 0; x < r.width(); ++x)
            line[x] = qRgba(qRed(line[x]), qGreen(line[x]), qBlue(line[x]), 255 - qAlpha(line[x]));
    }
    return r;
}
