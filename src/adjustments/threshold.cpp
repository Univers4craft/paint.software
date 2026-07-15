#include "threshold.h"

QImage Threshold::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int gray = static_cast<int>(0.299 * qRed(line[x]) + 0.587 * qGreen(line[x]) + 0.114 * qBlue(line[x]));
            int val = (gray >= m_threshold) ? 255 : 0;
            line[x] = qRgba(val, val, val, qAlpha(line[x]));
        }
    }
    return result;
}
