#include "highlightsshadows.h"
#include <cmath>

QImage HighlightsShadows::apply(const QImage &input) {
    QImage r = input.convertToFormat(QImage::Format_ARGB32);
    const double hi = m_highlights / 100.0;
    const double sh = m_shadows / 100.0;
    // Per-luminance LUT: shadows weight strong at low values, highlights at high.
    int lut[256];
    for (int i = 0; i < 256; ++i) {
        double v = i / 255.0;
        double shadowW = 1.0 - v;              // 1 at black -> 0 at white
        double highlightW = v;                 // 0 at black -> 1 at white
        // shadows>0 lifts dark tones; highlights<0 darkens bright tones.
        double adj = sh * shadowW * 0.6 + hi * highlightW * 0.6;
        double nv = qBound(0.0, v + adj, 1.0);
        lut[i] = static_cast<int>(nv * 255 + 0.5);
    }
    for (int y = 0; y < r.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(r.scanLine(y));
        for (int x = 0; x < r.width(); ++x) {
            // Apply on luminance, preserve hue by scaling channels equally.
            int rr = qRed(line[x]), gg = qGreen(line[x]), bb = qBlue(line[x]);
            int lum = qBound(0, (rr * 299 + gg * 587 + bb * 114) / 1000, 255);
            int nl = lut[lum];
            int delta = nl - lum;
            line[x] = qRgba(qBound(0, rr + delta, 255), qBound(0, gg + delta, 255),
                            qBound(0, bb + delta, 255), qAlpha(line[x]));
        }
    }
    return r;
}
