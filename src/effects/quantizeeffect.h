#pragma once
#include "effect.h"

class QuantizeEffect : public Effect {
public:
    QString name() const override { return "Quantize"; }
    QImage apply(const QImage &input) override;

    int colors() const { return m_colors; }
    void setColors(int c) { m_colors = c; }

private:
    int m_colors = 16;
};
