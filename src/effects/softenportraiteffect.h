#pragma once
#include "effect.h"

class SoftenPortraitEffect : public Effect {
public:
    QString name() const override { return "Soften Portrait"; }
    QImage apply(const QImage &input) override;

    int softness() const { return m_softness; }
    void setSoftness(int s) { m_softness = s; }
    int warmth() const { return m_warmth; }
    void setWarmth(int w) { m_warmth = w; }

private:
    int m_softness = 5;
    int m_warmth = 10;
};
