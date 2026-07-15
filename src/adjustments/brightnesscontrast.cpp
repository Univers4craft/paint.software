#include "brightnesscontrast.h"
#include <cmath>

QImage BrightnessContrast::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();

    double contrastFactor = (259.0 * (m_contrast + 255)) / (255.0 * (259 - m_contrast));

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = qRed(line[x]) + m_brightness;
            int g = qGreen(line[x]) + m_brightness;
            int b = qBlue(line[x]) + m_brightness;

            r = static_cast<int>(contrastFactor * (r - 128) + 128);
            g = static_cast<int>(contrastFactor * (g - 128) + 128);
            b = static_cast<int>(contrastFactor * (b - 128) + 128);

            line[x] = qRgba(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qAlpha(line[x]));
        }
    }
    return result;
}
