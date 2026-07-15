#pragma once
#include "adjustment.h"

class Threshold : public Adjustment {
public:
    QString name() const override { return "Threshold"; }
    QImage apply(const QImage &input) override;
    int threshold() const { return m_threshold; }
    void setThreshold(int t) { m_threshold = qBound(0, t, 255); }
private:
    int m_threshold = 128;
};
