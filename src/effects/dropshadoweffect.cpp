#include "dropshadoweffect.h"
#include <QPainter>
#include <vector>
#include <algorithm>

// Builds a soft shadow from the source's alpha channel, offset and blurred,
// then draws the original image on top.
QImage DropShadowEffect::apply(const QImage &input) {
    QImage src = input.convertToFormat(QImage::Format_ARGB32);
    const int w = src.width(), h = src.height();

    // 1) Extract alpha into a shadow mask.
    std::vector<int> a(w * h), tmp(w * h);
    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        for (int x = 0; x < w; ++x) a[y * w + x] = qAlpha(line[x]);
    }

    // 2) Separable box blur (3 passes ~ gaussian).
    const int rad = std::max(0, m_radius);
    auto blurH = [&](std::vector<int> &in, std::vector<int> &out) {
        const double inv = 1.0 / (2 * rad + 1);
        for (int y = 0; y < h; ++y) {
            long sum = 0;
            for (int k = -rad; k <= rad; ++k) sum += in[y * w + std::clamp(k, 0, w - 1)];
            for (int x = 0; x < w; ++x) {
                out[y * w + x] = static_cast<int>(sum * inv);
                int add = in[y * w + std::clamp(x + rad + 1, 0, w - 1)];
                int rem = in[y * w + std::clamp(x - rad, 0, w - 1)];
                sum += add - rem;
            }
        }
    };
    auto blurV = [&](std::vector<int> &in, std::vector<int> &out) {
        const double inv = 1.0 / (2 * rad + 1);
        for (int x = 0; x < w; ++x) {
            long sum = 0;
            for (int k = -rad; k <= rad; ++k) sum += in[std::clamp(k, 0, h - 1) * w + x];
            for (int y = 0; y < h; ++y) {
                out[y * w + x] = static_cast<int>(sum * inv);
                int add = in[std::clamp(y + rad + 1, 0, h - 1) * w + x];
                int rem = in[std::clamp(y - rad, 0, h - 1) * w + x];
                sum += add - rem;
            }
        }
    };
    if (rad > 0) {
        for (int p = 0; p < 3; ++p) { blurH(a, tmp); blurV(tmp, a); }
    }

    // 3) Compose shadow (offset) then the original on top.
    QImage result(w, h, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    QImage shadow(w, h, QImage::Format_ARGB32);
    shadow.fill(Qt::transparent);
    const double op = m_opacity / 100.0;
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(shadow.scanLine(y));
        for (int x = 0; x < w; ++x)
            line[x] = qRgba(0, 0, 0, qBound(0, static_cast<int>(a[y * w + x] * op), 255));
    }
    QPainter p(&result);
    p.drawImage(m_offsetX, m_offsetY, shadow);
    p.drawImage(0, 0, src);
    p.end();
    return result;
}
