#pragma once
#include "effect.h"

class TwistEffect : public Effect {
public:
    QString name() const override { return "Twist"; }
    QImage apply(const QImage &input) override;

    int amount() const { return m_amount; }
    void setAmount(int a) { m_amount = a; }

private:
    int m_amount = 45;
};
