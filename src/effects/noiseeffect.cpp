#include "noiseeffect.h"
#include <QRandomGenerator>

QImage NoiseEffect::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();
    auto *rng = QRandomGenerator::global();

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int noise = rng->bounded(-m_intensity, m_intensity + 1);
            int r = qBound(0, qRed(line[x]) + noise, 255);
            int g = qBound(0, qGreen(line[x]) + noise, 255);
            int b = qBound(0, qBlue(line[x]) + noise, 255);
            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }
    }
    return result;
}
