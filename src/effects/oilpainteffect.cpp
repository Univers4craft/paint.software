#include "oilpainteffect.h"
#include <vector>

QImage OilPaintEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    QImage result(src.size(), QImage::Format_ARGB32);
    int w = src.width(), h = src.height();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            std::vector<int> intensityCount(m_levels + 1, 0);
            std::vector<int> rSum(m_levels + 1, 0), gSum(m_levels + 1, 0), bSum(m_levels + 1, 0);

            for (int dy = -m_radius; dy <= m_radius; ++dy) {
                for (int dx = -m_radius; dx <= m_radius; ++dx) {
                    int nx = qBound(0, x + dx, w - 1);
                    int ny = qBound(0, y + dy, h - 1);
                    QRgb p = src.pixel(nx, ny);

                    int intensity = static_cast<int>((qRed(p) + qGreen(p) + qBlue(p)) / 3.0 * m_levels / 255.0);
                    intensity = qBound(0, intensity, m_levels);
                    intensityCount[intensity]++;
                    rSum[intensity] += qRed(p);
                    gSum[intensity] += qGreen(p);
                    bSum[intensity] += qBlue(p);
                }
            }

            int maxIdx = 0, maxCount = 0;
            for (int i = 0; i <= m_levels; ++i) {
                if (intensityCount[i] > maxCount) {
                    maxCount = intensityCount[i];
                    maxIdx = i;
                }
            }

            if (maxCount > 0) {
                result.setPixel(x, y, qRgba(
                    rSum[maxIdx] / maxCount,
                    gSum[maxIdx] / maxCount,
                    bSum[maxIdx] / maxCount,
                    qAlpha(src.pixel(x, y))));
            } else {
                result.setPixel(x, y, src.pixel(x, y));
            }
        }
    }
    return result;
}
