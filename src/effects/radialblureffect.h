#pragma once
#include "effect.h"

class RadialBlurEffect : public Effect {
public:
    QString name() const override { return "Radial Blur"; }
    QImage apply(const QImage &input) override;

    int angle() const { return m_angle; }
    void setAngle(int a) { m_angle = a; }
    QPoint center() const { return m_center; }
    void setCenter(QPoint c) { m_center = c; }

private:
    int m_angle = 2;
    QPoint m_center{0, 0};
};
