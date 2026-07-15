#pragma once
#include "adjustment.h"

class ColorBalance : public Adjustment {
public:
    QString name() const override { return "Color Balance"; }
    QImage apply(const QImage &input) override;
    int cyanRed() const { return m_cyanRed; }
    void setCyanRed(int v) { m_cyanRed = qBound(-100, v, 100); }
    int magentaGreen() const { return m_magentaGreen; }
    void setMagentaGreen(int v) { m_magentaGreen = qBound(-100, v, 100); }
    int yellowBlue() const { return m_yellowBlue; }
    void setYellowBlue(int v) { m_yellowBlue = qBound(-100, v, 100); }
private:
    int m_cyanRed = 0;
    int m_magentaGreen = 0;
    int m_yellowBlue = 0;
};
