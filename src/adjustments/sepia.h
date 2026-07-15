#pragma once
#include "adjustment.h"

class Sepia : public Adjustment {
public:
    QString name() const override { return "Sepia"; }
    QImage apply(const QImage &input) override;
    int intensity() const { return m_intensity; }
    void setIntensity(int i) { m_intensity = qBound(0, i, 100); }
private:
    int m_intensity = 80;
};
