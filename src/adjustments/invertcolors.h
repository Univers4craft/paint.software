#pragma once
#include "adjustment.h"

class InvertColors : public Adjustment {
public:
    QString name() const override { return "Invert Colors"; }
    QImage apply(const QImage &input) override;
};
