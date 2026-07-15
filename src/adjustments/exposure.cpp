#include "exposure.h"
#include <cmath>

QImage Exposure::apply(const QImage &input) {
    QImage r = input.convertToFormat(QImage::Format_ARGB32);
    const double factor = std::pow(2.0, m_exposure / 50.0);   // -100..100 -> 2^(-2..2)
    int lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = qBound(0, static_cast<int>(i * factor + 0.5), 255);
    for (int y = 0; y < r.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(r.scanLine(y));
        for (int x = 0; x < r.width(); ++x)
            line[x] = qRgba(lut[qRed(line[x])], lut[qGreen(line[x])], lut[qBlue(line[x])], qAlpha(line[x]));
    }
    return r;
}
