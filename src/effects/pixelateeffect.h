#pragma once
#include "effect.h"

class PixelateEffect : public Effect {
public:
    QString name() const override { return "Pixelate"; }
    QImage apply(const QImage &input) override;
    int cellSize() const { return m_cellSize; }
    void setCellSize(int s) { m_cellSize = qBound(2, s, 100); }
private:
    int m_cellSize = 8;
};
