#pragma once
#include "effect.h"

class GlowEffect : public Effect {
public:
    QString name() const override { return "Glow"; }
    QImage apply(const QImage &input) override;

    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = r; }
    int brightness() const { return m_brightness; }
    void setBrightness(int b) { m_brightness = b; }

private:
    int m_radius = 6;
    int m_brightness = 10;
};
