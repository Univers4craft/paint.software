#pragma once
#include "effect.h"

class TurbulenceEffect : public Effect {
public:
    QString name() const override { return "Turbulence"; }
    QImage apply(const QImage &input) override;

    int scale() const { return m_scale; }
    void setScale(int s) { m_scale = s; }
    int roughness() const { return m_roughness; }
    void setRoughness(int r) { m_roughness = r; }

private:
    int m_scale = 100;
    int m_roughness = 50;
};
