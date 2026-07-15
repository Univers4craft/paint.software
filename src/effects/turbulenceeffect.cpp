#include "turbulenceeffect.h"
#include <cmath>

QImage TurbulenceEffect::apply(const QImage &input) {
    // Render effect - generates a turbulence pattern at the input's dimensions.
    int w = input.width() > 0 ? input.width() : 800;
    int h = input.height() > 0 ? input.height() : 600;
    QImage result(w, h, QImage::Format_ARGB32);
    double freq = m_scale / 200.0;
    int octaves = qBound(1, m_roughness / 10, 8);

    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double val = 0;
            double amplitude = 1.0;
            double frequency = freq;
            for (int o = 0; o < octaves; ++o) {
                double nx = x * frequency * 0.01;
                double ny = y * frequency * 0.01;
                double noise = std::sin(nx * 1.5 + ny * 0.7) * std::cos(ny * 1.3 - nx * 0.4);
                noise += std::sin(nx * 2.3 - ny * 1.9) * 0.5;
                val += std::abs(noise) * amplitude;
                amplitude *= 0.5;
                frequency *= 2.0;
            }
            val = qBound(0.0, val * 0.3 + 0.35, 1.0);
            int c = static_cast<int>(val * 255);
            dst[x] = qRgba(c, c, c, 255);
        }
    }
    return result;
}
