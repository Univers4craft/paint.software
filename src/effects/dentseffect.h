#pragma once
#include "effect.h"

class DentsEffect : public Effect {
public:
    QString name() const override { return "Dents"; }
    QImage apply(const QImage &input) override;

    int scale() const { return m_scale; }
    void setScale(int s) { m_scale = s; }
    int refraction() const { return m_refraction; }
    void setRefraction(int r) { m_refraction = r; }

private:
    int m_scale = 25;
    int m_refraction = 50;
};
