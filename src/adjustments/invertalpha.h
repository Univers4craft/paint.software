#pragma once
#include "adjustment.h"

class InvertAlpha : public Adjustment {
public:
    QString name() const override { return "Invert Alpha"; }
    QImage apply(const QImage &input) override;
};
