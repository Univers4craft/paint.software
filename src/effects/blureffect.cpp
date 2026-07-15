#include "blureffect.h"
#include <cmath>
#include <vector>

QImage BlurEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    int w = src.width(), h = src.height();
    QImage result(w, h, QImage::Format_ARGB32);

    // Box blur approximation (3 passes)
    std::vector<QRgb> temp(w * h);
    const QRgb *srcBits = reinterpret_cast<const QRgb*>(src.constBits());

    auto getPixel = [&](const QRgb *data, int x, int y) -> QRgb {
        x = qBound(0, x, w - 1);
        y = qBound(0, y, h - 1);
        return data[y * w + x];
    };

    auto boxBlurH = [&](const QRgb *in, QRgb *out, int radius) {
        double iarr = 1.0 / (radius + radius + 1);
        for (int y = 0; y < h; ++y) {
            double rAcc = 0, gAcc = 0, bAcc = 0, aAcc = 0;
            for (int x = -radius; x <= radius; ++x) {
                QRgb p = getPixel(in, x, y);
                rAcc += qRed(p); gAcc += qGreen(p); bAcc += qBlue(p); aAcc += qAlpha(p);
            }
            for (int x = 0; x < w; ++x) {
                out[y * w + x] = qRgba(
                    qBound(0, (int)(rAcc * iarr), 255),
                    qBound(0, (int)(gAcc * iarr), 255),
                    qBound(0, (int)(bAcc * iarr), 255),
                    qBound(0, (int)(aAcc * iarr), 255));
                QRgb add = getPixel(in, x + radius + 1, y);
                QRgb rem = getPixel(in, x - radius, y);
                rAcc += qRed(add) - qRed(rem);
                gAcc += qGreen(add) - qGreen(rem);
                bAcc += qBlue(add) - qBlue(rem);
                aAcc += qAlpha(add) - qAlpha(rem);
            }
        }
    };

    auto boxBlurV = [&](const QRgb *in, QRgb *out, int radius) {
        double iarr = 1.0 / (radius + radius + 1);
        for (int x = 0; x < w; ++x) {
            double rAcc = 0, gAcc = 0, bAcc = 0, aAcc = 0;
            for (int y = -radius; y <= radius; ++y) {
                QRgb p = getPixel(in, x, y);
                rAcc += qRed(p); gAcc += qGreen(p); bAcc += qBlue(p); aAcc += qAlpha(p);
            }
            for (int y = 0; y < h; ++y) {
                out[y * w + x] = qRgba(
                    qBound(0, (int)(rAcc * iarr), 255),
                    qBound(0, (int)(gAcc * iarr), 255),
                    qBound(0, (int)(bAcc * iarr), 255),
                    qBound(0, (int)(aAcc * iarr), 255));
                QRgb add = getPixel(in, x, y + radius + 1);
                QRgb rem = getPixel(in, x, y - radius);
                rAcc += qRed(add) - qRed(rem);
                gAcc += qGreen(add) - qGreen(rem);
                bAcc += qBlue(add) - qBlue(rem);
                aAcc += qAlpha(add) - qAlpha(rem);
            }
        }
    };

    std::vector<QRgb> buf1(srcBits, srcBits + w * h);
    std::vector<QRgb> buf2(w * h);

    for (int pass = 0; pass < 3; ++pass) {
        boxBlurH(buf1.data(), buf2.data(), m_radius);
        boxBlurV(buf2.data(), buf1.data(), m_radius);
    }

    memcpy(result.bits(), buf1.data(), w * h * sizeof(QRgb));
    return result;
}
