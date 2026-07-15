#pragma once
#include "effect.h"

class EmbossEffect : public Effect {
public:
    QString name() const override { return "Emboss"; }
    QImage apply(const QImage &input) override;
    double angle() const { return m_angle; }
    void setAngle(double a) { m_angle = a; }
private:
    double m_angle = 135.0;
};
