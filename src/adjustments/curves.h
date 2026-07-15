#pragma once
#include "adjustment.h"
#include <QVector>
#include <QPointF>

class Curves : public Adjustment {
public:
    QString name() const override { return "Curves"; }
    QImage apply(const QImage &input) override;

    void setControlPoints(const QVector<QPointF> &points) { m_points = points; buildLUT(); }
    QVector<QPointF> controlPoints() const { return m_points; }

private:
    void buildLUT();
    QVector<QPointF> m_points = {{0,0},{64,64},{128,128},{192,192},{255,255}};
    int m_lut[256];
};
