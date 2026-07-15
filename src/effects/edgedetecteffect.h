#pragma once
#include "effect.h"

class EdgeDetectEffect : public Effect {
public:
    QString name() const override { return "Edge Detect"; }
    QImage apply(const QImage &input) override;
};
