#pragma once
#include "adjustment.h"

class TemperatureTint : public Adjustment {
public:
    QString name() const override { return "Temperature / Tint"; }
    void setTemperature(int t) { m_temp = t; }   // -100 (cool/blue) .. 100 (warm/orange)
    void setTint(int t)        { m_tint = t; }    // -100 (green) .. 100 (magenta)
    QImage apply(const QImage &input) override;
private:
    int m_temp = 0;
    int m_tint = 0;
};
