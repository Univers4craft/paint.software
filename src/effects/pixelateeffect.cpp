#include "pixelateeffect.h"

QImage PixelateEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(src.size(), QImage::Format_ARGB32);
    int w = src.width(), h = src.height();

    for (int cy = 0; cy < h; cy += m_cellSize) {
        for (int cx = 0; cx < w; cx += m_cellSize) {
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0, count = 0;
            int endX = qMin(cx + m_cellSize, w);
            int endY = qMin(cy + m_cellSize, h);

            for (int y = cy; y < endY; ++y) {
                for (int x = cx; x < endX; ++x) {
                    QRgb p = src.pixel(x, y);
                    rSum += qRed(p); gSum += qGreen(p); bSum += qBlue(p); aSum += qAlpha(p);
                    ++count;
                }
            }

            QRgb avg = qRgba(rSum/count, gSum/count, bSum/count, aSum/count);
            for (int y = cy; y < endY; ++y)
                for (int x = cx; x < endX; ++x)
                    result.setPixel(x, y, avg);
        }
    }
    return result;
}
