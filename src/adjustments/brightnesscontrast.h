#pragma once
#include "adjustment.h"

class BrightnessContrast : public Adjustment {
public:
    QString name() const override { return "Brightness / Contrast"; }
    QImage apply(const QImage &input) override;
    int brightness() const { return m_brightness; }
    void setBrightness(int b) { m_brightness = qBound(-100, b, 100); }
    int contrast() const { return m_contrast; }
    void setContrast(int c) { m_contrast = qBound(-100, c, 100); }
private:
    int m_brightness = 0;
    int m_contrast = 0;
};
