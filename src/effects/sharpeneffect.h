#pragma once
#include "effect.h"

class SharpenEffect : public Effect {
public:
    QString name() const override { return "Sharpen"; }
    QImage apply(const QImage &input) override;
    int amount() const { return m_amount; }
    void setAmount(int a) { m_amount = qBound(1, a, 100); }
private:
    int m_amount = 50;
};
