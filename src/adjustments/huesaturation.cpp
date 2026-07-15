#include "huesaturation.h"
#include <QColor>

QImage HueSaturation::apply(const QImage &input) {
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QColor c = QColor::fromRgba(line[x]);
            // hsvHue() is -1 for achromatic (grey) pixels; treat that undefined
            // hue as 0 instead of letting -1 + m_hue wrap to ~359 (a red tint).
            int origHue = c.hsvHue();
            if (origHue < 0) origHue = 0;
            int h2 = origHue + m_hue;
            if (h2 < 0) h2 += 360;
            if (h2 >= 360) h2 -= 360;

            // An achromatic (grey) pixel has no hue to amplify, so leave its
            // saturation at 0 — otherwise cranking the slider tints it red.
            int origSat = c.hsvSaturation();
            int s = (origSat == 0) ? 0 : origSat + static_cast<int>(m_saturation * 2.55);
            int v = c.value() + static_cast<int>(m_lightness * 2.55);

            QColor adjusted = QColor::fromHsv(
                qBound(0, h2, 359),
                qBound(0, s, 255),
                qBound(0, v, 255),
                c.alpha());
            line[x] = adjusted.rgba();
        }
    }
    return result;
}
