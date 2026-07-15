#pragma once
#include "adjustment.h"

class Posterize : public Adjustment {
public:
    QString name() const override { return "Posterize"; }
    QImage apply(const QImage &input) override;
    int levels() const { return m_levels; }
    void setLevels(int l) { m_levels = qBound(2, l, 64); }
private:
    int m_levels = 4;
};
