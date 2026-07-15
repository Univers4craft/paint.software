#include "levels.h"
#include <cmath>

QImage Levels::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();

    // Build lookup table
    int lut[256];
    double inputRange = std::max(1.0, (double)(m_inputWhite - m_inputBlack));
    double outputRange = m_outputWhite - m_outputBlack;

    for (int i = 0; i < 256; ++i) {
        double normalized = qBound(0.0, (i - m_inputBlack) / inputRange, 1.0);
        double gammaCorrected = std::pow(normalized, 1.0 / m_gamma);
        lut[i] = qBound(0, static_cast<int>(m_outputBlack + gammaCorrected * outputRange), 255);
    }

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            line[x] = qRgba(lut[qRed(line[x])], lut[qGreen(line[x])], lut[qBlue(line[x])], qAlpha(line[x]));
        }
    }
    return result;
}
