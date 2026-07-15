#pragma once
#include "effect.h"

class ReliefEffect : public Effect {
public:
    QString name() const override { return "Relief"; }
    QImage apply(const QImage &input) override;

    int angle() const { return m_angle; }
    void setAngle(int a) { m_angle = a; }

private:
    int m_angle = 45;
};
