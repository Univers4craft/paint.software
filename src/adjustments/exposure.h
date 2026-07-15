#pragma once
#include "adjustment.h"

class Exposure : public Adjustment {
public:
    QString name() const override { return "Exposure"; }
    // Stops of exposure, -100..100 (mapped to roughly -2..+2 EV).
    void setExposure(int e) { m_exposure = e; }
    QImage apply(const QImage &input) override;
private:
    int m_exposure = 0;
};
