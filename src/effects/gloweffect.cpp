#include "gloweffect.h"
#include "blureffect.h"
#include <algorithm>

QImage GlowEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);

    // Create blurred version
    BlurEffect blur;
    blur.setRadius(m_radius);
    QImage blurred = blur.apply(img);

    // Screen blend: result = 1 - (1-a)(1-b)
    QImage result = img.copy();
    int w = img.width(), h = img.height();
    double bright = 1.0 + m_brightness / 10.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb *srcA = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        const QRgb *srcB = reinterpret_cast<const QRgb*>(blurred.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = 255 - (int)((255 - qRed(srcA[x])) * (255 - qRed(srcB[x]) * bright) / 255.0);
            int g = 255 - (int)((255 - qGreen(srcA[x])) * (255 - qGreen(srcB[x]) * bright) / 255.0);
            int b = 255 - (int)((255 - qBlue(srcA[x])) * (255 - qBlue(srcB[x]) * bright) / 255.0);
            dst[x] = qRgba(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qAlpha(srcA[x]));
        }
    }
    return result;
}
