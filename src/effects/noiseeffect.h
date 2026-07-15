#pragma once
#include "effect.h"

class NoiseEffect : public Effect {
public:
    QString name() const override { return "Add Noise"; }
    QImage apply(const QImage &input) override;
    int intensity() const { return m_intensity; }
    void setIntensity(int i) { m_intensity = qBound(1, i, 100); }
private:
    int m_intensity = 30;
};
