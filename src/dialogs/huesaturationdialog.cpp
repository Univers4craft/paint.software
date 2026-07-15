#include "huesaturationdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLinearGradient>
#include <QPainter>

// Custom slider that draws a gradient background (like Paint.NET)
class GradientSlider : public QSlider {
public:
    enum GradientType { Hue, Saturation, Lightness };

    GradientSlider(GradientType type, QWidget *parent = nullptr)
        : QSlider(Qt::Horizontal, parent), m_type(type) {
        setFixedHeight(24);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect grooveRect(4, 6, width() - 8, 12);

        // Draw gradient background
        QLinearGradient grad(grooveRect.left(), 0, grooveRect.right(), 0);
        if (m_type == Hue) {
            for (int i = 0; i <= 6; ++i) {
                QColor c = QColor::fromHsvF(i / 6.0, 1.0, 1.0);
                grad.setColorAt(i / 6.0, c);
            }
        } else if (m_type == Saturation) {
            grad.setColorAt(0, QColor::fromHsvF(0, 0, 0.7));
            grad.setColorAt(0.5, QColor::fromHsvF(0.33, 0.5, 0.85));
            grad.setColorAt(1.0, QColor::fromHsvF(0.66, 1.0, 1.0));
        } else {
            grad.setColorAt(0, Qt::black);
            grad.setColorAt(0.5, Qt::gray);
            grad.setColorAt(1.0, Qt::white);
        }

        p.setBrush(grad);
        p.setPen(QPen(QColor(120, 120, 120), 1));
        p.drawRoundedRect(grooveRect, 2, 2);

        // Draw handle
        double ratio = (double)(value() - minimum()) / (maximum() - minimum());
        int handleX = grooveRect.left() + ratio * grooveRect.width();
        QRect handle(handleX - 5, 3, 10, 18);
        p.setBrush(QColor(80, 140, 210));
        p.setPen(QPen(QColor(40, 80, 140), 1));
        p.drawRoundedRect(handle, 2, 2);

        // White triangle indicator
        p.setPen(Qt::white);
        p.drawLine(handleX, 6, handleX, 18);
    }

private:
    GradientType m_type;
};

HueSaturationDialog::HueSaturationDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Hue / Saturation");
    setFixedSize(420, 220);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    auto *grid = new QGridLayout;
    grid->setSpacing(8);
    grid->setColumnStretch(1, 1);

    // Hue
    auto *hueLabel = new QLabel("Hue");
    hueLabel->setFixedWidth(70);
    grid->addWidget(hueLabel, 0, 0);
    m_hueSlider = new GradientSlider(GradientSlider::Hue);
    m_hueSlider->setRange(-180, 180);
    m_hueSlider->setValue(0);
    grid->addWidget(m_hueSlider, 0, 1);
    m_hueSpin = new QSpinBox;
    m_hueSpin->setRange(-180, 180);
    m_hueSpin->setFixedWidth(60);
    grid->addWidget(m_hueSpin, 0, 2);
    auto *hueReset = new QPushButton("↺");
    hueReset->setFixedSize(22, 22);
    grid->addWidget(hueReset, 0, 3);

    // Saturation
    auto *satLabel = new QLabel("Saturation");
    grid->addWidget(satLabel, 1, 0);
    m_satSlider = new GradientSlider(GradientSlider::Saturation);
    m_satSlider->setRange(0, 200);
    m_satSlider->setValue(100);
    grid->addWidget(m_satSlider, 1, 1);
    m_satSpin = new QSpinBox;
    m_satSpin->setRange(0, 200);
    m_satSpin->setFixedWidth(60);
    m_satSpin->setValue(100);
    grid->addWidget(m_satSpin, 1, 2);
    auto *satReset = new QPushButton("↺");
    satReset->setFixedSize(22, 22);
    grid->addWidget(satReset, 1, 3);

    // Lightness
    auto *lightLabel = new QLabel("Lightness");
    grid->addWidget(lightLabel, 2, 0);
    m_lightnessSlider = new GradientSlider(GradientSlider::Lightness);
    m_lightnessSlider->setRange(-100, 100);
    m_lightnessSlider->setValue(0);
    grid->addWidget(m_lightnessSlider, 2, 1);
    m_lightnessSpin = new QSpinBox;
    m_lightnessSpin->setRange(-100, 100);
    m_lightnessSpin->setFixedWidth(60);
    grid->addWidget(m_lightnessSpin, 2, 2);
    auto *lightReset = new QPushButton("↺");
    lightReset->setFixedSize(22, 22);
    grid->addWidget(lightReset, 2, 3);

    mainLayout->addLayout(grid);

    // OK / Cancel buttons
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *okBtn = new QPushButton("OK");
    okBtn->setFixedWidth(80);
    auto *cancelBtn = new QPushButton("Cancel");
    cancelBtn->setFixedWidth(80);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    // Connections
    connect(m_hueSlider, &QSlider::valueChanged, m_hueSpin, &QSpinBox::setValue);
    connect(m_hueSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_hueSlider, &QSlider::setValue);
    connect(m_satSlider, &QSlider::valueChanged, m_satSpin, &QSpinBox::setValue);
    connect(m_satSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_satSlider, &QSlider::setValue);
    connect(m_lightnessSlider, &QSlider::valueChanged, m_lightnessSpin, &QSpinBox::setValue);
    connect(m_lightnessSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_lightnessSlider, &QSlider::setValue);

    connect(hueReset, &QPushButton::clicked, this, [this]() { m_hueSlider->setValue(0); });
    connect(satReset, &QPushButton::clicked, this, [this]() { m_satSlider->setValue(100); });
    connect(lightReset, &QPushButton::clicked, this, [this]() { m_lightnessSlider->setValue(0); });

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}
