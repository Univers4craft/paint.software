#pragma once
#include "effect.h"

class MorphologyEffect : public Effect {
public:
    QString name() const override { return "Morphology"; }
    QImage apply(const QImage &input) override;

    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = r; }
    bool dilate() const { return m_dilate; }
    void setDilate(bool d) { m_dilate = d; }

private:
    int m_radius = 1;
    bool m_dilate = true;
};
