#pragma once
#include "effect.h"

class SurfaceBlurEffect : public Effect {
public:
    QString name() const override { return "Surface Blur"; }
    QImage apply(const QImage &input) override;

    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = qMax(0, r); }
    int threshold() const { return m_threshold; }
    void setThreshold(int t) { m_threshold = qMax(1, t); }   // used as a divisor

private:
    int m_radius = 5;
    int m_threshold = 15;
};
