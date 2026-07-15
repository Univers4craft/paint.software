#pragma once
#include "effect.h"

class FrostedGlassEffect : public Effect {
public:
    QString name() const override { return "Frosted Glass"; }
    QImage apply(const QImage &input) override;

    int amount() const { return m_amount; }
    void setAmount(int a) { m_amount = qMax(0, a); }

private:
    int m_amount = 3;
};
