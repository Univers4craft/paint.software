#pragma once
#include "adjustment.h"

class Levels : public Adjustment {
public:
    QString name() const override { return "Levels"; }
    QImage apply(const QImage &input) override;
    int inputBlack() const { return m_inputBlack; }
    void setInputBlack(int v) { m_inputBlack = qBound(0, v, 255); }
    int inputWhite() const { return m_inputWhite; }
    void setInputWhite(int v) { m_inputWhite = qBound(0, v, 255); }
    double gamma() const { return m_gamma; }
    void setGamma(double g) { m_gamma = qBound(0.1, g, 10.0); }
    int outputBlack() const { return m_outputBlack; }
    void setOutputBlack(int v) { m_outputBlack = qBound(0, v, 255); }
    int outputWhite() const { return m_outputWhite; }
    void setOutputWhite(int v) { m_outputWhite = qBound(0, v, 255); }
private:
    int m_inputBlack = 0;
    int m_inputWhite = 255;
    double m_gamma = 1.0;
    int m_outputBlack = 0;
    int m_outputWhite = 255;
};
