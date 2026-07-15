#pragma once
#include "effect.h"

class CrystalizeEffect : public Effect {
public:
    QString name() const override { return "Crystalize"; }
    QImage apply(const QImage &input) override;

    int cellSize() const { return m_cellSize; }
    void setCellSize(int s) { m_cellSize = qBound(1, s, 512); }   // cap to avoid overflow

private:
    int m_cellSize = 10;
};
