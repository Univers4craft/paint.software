#pragma once
#include "effect.h"

class PolarInversionEffect : public Effect {
public:
    QString name() const override { return "Polar Inversion"; }
    QImage apply(const QImage &input) override;

    int amount() const { return m_amount; }
    void setAmount(int a) { m_amount = a; }

private:
    int m_amount = 50;
};
