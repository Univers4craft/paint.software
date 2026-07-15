#pragma once
#include "effect.h"

class OutlineEffect : public Effect {
public:
    QString name() const override { return "Outline"; }
    QImage apply(const QImage &input) override;

    int thickness() const { return m_thickness; }
    void setThickness(int t) { m_thickness = t; }
    int intensity() const { return m_intensity; }
    void setIntensity(int i) { m_intensity = i; }

private:
    int m_thickness = 3;
    int m_intensity = 50;
};
