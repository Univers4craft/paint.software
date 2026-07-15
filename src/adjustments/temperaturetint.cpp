#include "temperaturetint.h"

QImage TemperatureTint::apply(const QImage &input) {
    QImage r = input.convertToFormat(QImage::Format_ARGB32);
    // Warm = +red / -blue ; Tint magenta = +red+blue / -green.
    const int t = m_temp;        // scaled below
    const int g = m_tint;
    const int dR = t * 60 / 100 + g * 25 / 100;
    const int dG = -g * 50 / 100;
    const int dB = -t * 60 / 100 + g * 25 / 100;
    for (int y = 0; y < r.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(r.scanLine(y));
        for (int x = 0; x < r.width(); ++x) {
            line[x] = qRgba(qBound(0, qRed(line[x]) + dR, 255),
                            qBound(0, qGreen(line[x]) + dG, 255),
                            qBound(0, qBlue(line[x]) + dB, 255),
                            qAlpha(line[x]));
        }
    }
    return r;
}
