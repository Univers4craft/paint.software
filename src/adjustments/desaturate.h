#pragma once
#include "adjustment.h"

// Paint.NET "Black and White": desaturate to luminance grey, keeping alpha.
class Desaturate : public Adjustment {
public:
    QString name() const override { return "Black and White"; }
    QImage apply(const QImage &input) override;
};
