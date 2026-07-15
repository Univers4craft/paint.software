#pragma once
#include "effect.h"

class FragmentEffect : public Effect {
public:
    QString name() const override { return "Fragment"; }
    QImage apply(const QImage &input) override;

    int fragments() const { return m_fragments; }
    void setFragments(int f) { m_fragments = qMax(1, f); }   // used as a divisor
    int distance() const { return m_distance; }
    void setDistance(int d) { m_distance = qMax(0, d); }

private:
    int m_fragments = 4;
    int m_distance = 8;
};
