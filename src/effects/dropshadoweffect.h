#pragma once
#include "effect.h"
#include <QColor>

class DropShadowEffect : public Effect {
public:
    QString name() const override { return "Drop Shadow"; }
    void setRadius(int r)   { m_radius = r; }      // blur radius
    void setOffsetX(int x)  { m_offsetX = x; }
    void setOffsetY(int y)  { m_offsetY = y; }
    void setOpacity(int o)  { m_opacity = o; }      // 0..100
    QImage apply(const QImage &input) override;
private:
    int m_radius = 8;
    int m_offsetX = 6;
    int m_offsetY = 6;
    int m_opacity = 70;
};
