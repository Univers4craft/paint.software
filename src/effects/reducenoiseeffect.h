#pragma once
#include "effect.h"

class ReduceNoiseEffect : public Effect {
public:
    QString name() const override { return "Reduce Noise"; }
    QImage apply(const QImage &input) override;

    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = r; }
    int strength() const { return m_strength; }
    void setStrength(int s) { m_strength = qMax(1, s); }

private:
    int m_radius = 2;
    int m_strength = 40;
};
