#pragma once
#include "effect.h"

class MotionBlurEffect : public Effect {
public:
    QString name() const override { return "Motion Blur"; }
    QImage apply(const QImage &input) override;
    double angle() const { return m_angle; }
    void setAngle(double a) { m_angle = a; }
    int distance() const { return m_distance; }
    void setDistance(int d) { m_distance = qBound(1, d, 100); }
private:
    double m_angle = 0;
    int m_distance = 10;
};
