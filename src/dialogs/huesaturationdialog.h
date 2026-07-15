#pragma once

#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

class HueSaturationDialog : public QDialog {
    Q_OBJECT
public:
    explicit HueSaturationDialog(QWidget *parent = nullptr);

    int hue() const { return m_hueSlider->value(); }
    int saturation() const { return m_satSlider->value(); }
    int lightness() const { return m_lightnessSlider->value(); }

private:
    QSlider *m_hueSlider;
    QSpinBox *m_hueSpin;
    QSlider *m_satSlider;
    QSpinBox *m_satSpin;
    QSlider *m_lightnessSlider;
    QSpinBox *m_lightnessSpin;
};
