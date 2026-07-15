#pragma once
#include "effect.h"

class OilPaintEffect : public Effect {
public:
    QString name() const override { return "Oil Painting"; }
    QImage apply(const QImage &input) override;
    int radius() const { return m_radius; }
    void setRadius(int r) { m_radius = qBound(1, r, 10); }
    int levels() const { return m_levels; }
    void setLevels(int l) { m_levels = qBound(2, l, 64); }
private:
    int m_radius = 4;
    int m_levels = 20;
};
