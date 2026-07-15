#pragma once
#include "effect.h"

class VignetteEffect : public Effect {
public:
    QString name() const override { return "Vignette"; }
    QImage apply(const QImage &input) override;

    int amount() const { return m_amount; }
    void setAmount(int a) { m_amount = a; }
    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = r; }

private:
    int m_amount = 50;
    int m_radius = 80;
};
