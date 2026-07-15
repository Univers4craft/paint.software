#pragma once
#include "effect.h"

class TileEffect : public Effect {
public:
    QString name() const override { return "Tile Reflection"; }
    QImage apply(const QImage &input) override;

    int tileSize() const { return m_tileSize; }
    void setTileSize(int s) { m_tileSize = qMax(1, s); }
    int rotation() const { return m_rotation; }
    void setRotation(int r) { m_rotation = r; }

private:
    int m_tileSize = 40;
    int m_rotation = 30;
};
