#pragma once
#include "effect.h"

class MedianEffect : public Effect {
public:
    QString name() const override { return "Median"; }
    QImage apply(const QImage &input) override;

    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = qMax(0, r); }

private:
    int m_radius = 1;
};
