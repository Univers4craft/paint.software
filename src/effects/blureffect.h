#pragma once
#include "effect.h"

class BlurEffect : public Effect {
public:
    QString name() const override { return "Gaussian Blur"; }
    QImage apply(const QImage &input) override;

    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = qBound(1, r, 100); }

private:
    int m_radius = 5;
};
