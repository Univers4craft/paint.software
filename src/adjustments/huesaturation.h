#pragma once
#include "adjustment.h"

class HueSaturation : public Adjustment {
public:
    QString name() const override { return "Hue / Saturation"; }
    QImage apply(const QImage &input) override;
    int hue() const { return m_hue; }
    void setHue(int h) { m_hue = h % 360; }
    int saturation() const { return m_saturation; }
    void setSaturation(int s) { m_saturation = qBound(-100, s, 100); }
    int lightness() const { return m_lightness; }
    void setLightness(int l) { m_lightness = qBound(-100, l, 100); }
private:
    int m_hue = 0;
    int m_saturation = 0;
    int m_lightness = 0;
};
