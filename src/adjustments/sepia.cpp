#include "sepia.h"

QImage Sepia::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();
    double factor = m_intensity / 100.0;

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
            int sr = static_cast<int>(r * 0.393 + g * 0.769 + b * 0.189);
            int sg = static_cast<int>(r * 0.349 + g * 0.686 + b * 0.168);
            int sb = static_cast<int>(r * 0.272 + g * 0.534 + b * 0.131);

            int fr = static_cast<int>(r + (sr - r) * factor);
            int fg = static_cast<int>(g + (sg - g) * factor);
            int fb = static_cast<int>(b + (sb - b) * factor);

            line[x] = qRgba(qBound(0, fr, 255), qBound(0, fg, 255), qBound(0, fb, 255), qAlpha(line[x]));
        }
    }
    return result;
}
