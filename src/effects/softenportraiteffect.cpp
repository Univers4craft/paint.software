#include "softenportraiteffect.h"
#include "blureffect.h"

QImage SoftenPortraitEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);

    BlurEffect blur;
    blur.setRadius(m_softness);
    QImage blurred = blur.apply(img);

    QImage result = img.copy();
    int w = img.width(), h = img.height();
    double warmF = m_warmth / 100.0;

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb *orig = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        const QRgb *blur2 = reinterpret_cast<const QRgb*>(blurred.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            // Blend original with blurred (soft focus)
            int r = (qRed(orig[x]) + qRed(blur2[x])) / 2;
            int g = (qGreen(orig[x]) + qGreen(blur2[x])) / 2;
            int b = (qBlue(orig[x]) + qBlue(blur2[x])) / 2;
            // Add warmth (slight red/yellow shift)
            r = qBound(0, r + (int)(warmF * 20), 255);
            g = qBound(0, g + (int)(warmF * 8), 255);
            b = qBound(0, b - (int)(warmF * 5), 255);
            dst[x] = qRgba(r, g, b, qAlpha(orig[x]));
        }
    }
    return result;
}
