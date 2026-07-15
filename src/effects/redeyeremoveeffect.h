#pragma once
#include "effect.h"

class RedEyeRemoveEffect : public Effect {
public:
    QString name() const override { return "Red Eye Removal"; }
    QImage apply(const QImage &input) override;

    int tolerance() const { return m_tolerance; }
    void setTolerance(int t) { m_tolerance = t; }
    int saturation() const { return m_saturation; }
    void setSaturation(int s) { m_saturation = s; }

private:
    int m_tolerance = 70;
    int m_saturation = 80;
};
