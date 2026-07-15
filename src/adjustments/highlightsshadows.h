#pragma once
#include "adjustment.h"

class HighlightsShadows : public Adjustment {
public:
    QString name() const override { return "Highlights / Shadows"; }
    void setHighlights(int h) { m_highlights = h; }   // -100..100 (negative darkens highlights)
    void setShadows(int s)    { m_shadows = s; }      // -100..100 (positive lifts shadows)
    QImage apply(const QImage &input) override;
private:
    int m_highlights = 0;
    int m_shadows = 0;
};
