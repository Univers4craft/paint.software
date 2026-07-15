#include "curves.h"
#include <algorithm>

void Curves::buildLUT() {
    // Guarantee at least the two endpoints so the interpolation below never
    // indexes an empty/one-element list.
    if (m_points.size() < 2) {
        m_points.clear();
        m_points.append(QPointF(0, 0));
        m_points.append(QPointF(255, 255));
    }
    // Simple linear interpolation between control points
    std::sort(m_points.begin(), m_points.end(), [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

    for (int i = 0; i < 256; ++i) {
        // Clamp (hold) outside the control-point x-range instead of extrapolating
        // across the whole curve, which produced wrong values below/above the ends.
        if (i <= m_points.first().x()) {
            m_lut[i] = qBound(0, static_cast<int>(m_points.first().y()), 255);
            continue;
        }
        if (i >= m_points.last().x()) {
            m_lut[i] = qBound(0, static_cast<int>(m_points.last().y()), 255);
            continue;
        }
        // Find surrounding control points
        int lo = 0, hi = m_points.size() - 1;
        for (int j = 0; j < m_points.size() - 1; ++j) {
            if (m_points[j].x() <= i && m_points[j+1].x() >= i) {
                lo = j; hi = j + 1; break;
            }
        }

        double range = m_points[hi].x() - m_points[lo].x();
        double t = (range > 0) ? (i - m_points[lo].x()) / range : 0;
        double val = m_points[lo].y() + t * (m_points[hi].y() - m_points[lo].y());
        m_lut[i] = qBound(0, static_cast<int>(val), 255);
    }
}

QImage Curves::apply(const QImage &input) {
    buildLUT();
    QImage result = input.convertToFormat(QImage::Format_ARGB32);
    int w = result.width(), h = result.height();

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            line[x] = qRgba(m_lut[qRed(line[x])], m_lut[qGreen(line[x])], m_lut[qBlue(line[x])], qAlpha(line[x]));
        }
    }
    return result;
}
