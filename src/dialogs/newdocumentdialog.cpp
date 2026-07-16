#include "newdocumentdialog.h"
#include "../i18n.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QColorDialog>

NewDocumentDialog::NewDocumentDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Nouveau");
    setFixedWidth(360);
    m_ratio = 800.0 / 600.0;
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // Préréglage
    auto *presetLayout = new QHBoxLayout;
    presetLayout->addWidget(new QLabel(TR("Préréglage :")));
    m_presetCombo = new QComboBox;
    m_presetCombo->addItem(TR("Personnalisé"));
    m_presetCombo->addItem("640 \u00d7 480");
    m_presetCombo->addItem("800 \u00d7 600");
    m_presetCombo->addItem("1024 \u00d7 768");
    m_presetCombo->addItem("1280 \u00d7 720 (HD)");
    m_presetCombo->addItem("1920 \u00d7 1080 (Full HD)");
    m_presetCombo->addItem("2560 \u00d7 1440 (2K)");
    m_presetCombo->addItem("3840 \u00d7 2160 (4K)");
    m_presetCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    presetLayout->addWidget(m_presetCombo);
    layout->addLayout(presetLayout);

    // Section taille
    auto *sizeGroup = new QGroupBox(TR("Taille de l'image"));
    auto *sizeGrid = new QGridLayout(sizeGroup);
    sizeGrid->setSpacing(4);

    sizeGrid->addWidget(new QLabel(TR("Largeur :")), 0, 0);
    m_widthSpin = new QSpinBox;
    m_widthSpin->setRange(1, 65535);
    m_widthSpin->setValue(800);
    sizeGrid->addWidget(m_widthSpin, 0, 1);
    sizeGrid->addWidget(new QLabel("pixels"), 0, 2);

    sizeGrid->addWidget(new QLabel(TR("Hauteur :")), 1, 0);
    m_heightSpin = new QSpinBox;
    m_heightSpin->setRange(1, 65535);
    m_heightSpin->setValue(600);
    sizeGrid->addWidget(m_heightSpin, 1, 1);
    sizeGrid->addWidget(new QLabel("pixels"), 1, 2);

    sizeGrid->addWidget(new QLabel(TR("Résolution :")), 2, 0);
    m_resolutionSpin = new QSpinBox;
    m_resolutionSpin->setRange(1, 9999);
    m_resolutionSpin->setValue(96);
    m_resolutionSpin->setSuffix(" dpi");
    sizeGrid->addWidget(m_resolutionSpin, 2, 1);

    m_aspectRatio = new QCheckBox(TR("Conserver le ratio"));
    sizeGrid->addWidget(m_aspectRatio, 3, 0, 1, 3);

    layout->addWidget(sizeGroup);

    // Couleur de fond
    auto *bgLayout = new QHBoxLayout;
    bgLayout->addWidget(new QLabel(TR("Arrière-plan :")));
    m_colorBtn = new QPushButton;
    m_colorBtn->setFixedSize(80, 24);
    m_colorBtn->setStyleSheet("background-color: white; border: 1px solid #aaa;");
    bgLayout->addWidget(m_colorBtn);
    auto *colorLabel = new QLabel("Blanc");
    bgLayout->addWidget(colorLabel);
    bgLayout->addStretch();
    layout->addLayout(bgLayout);

    // Infos taille
    m_sizeInfoLabel = new QLabel;
    m_sizeInfoLabel->setStyleSheet("color: #555; font-size: 11px;");
    m_sizeInfoLabel->setText(QString("%1 \u00d7 %2 pixels").arg(800).arg(600));
    layout->addWidget(m_sizeInfoLabel);

    // OK / Annuler
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    // Connections
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewDocumentDialog::onPresetChanged);
    connect(m_colorBtn, &QPushButton::clicked, this, &NewDocumentDialog::onChooseColor);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NewDocumentDialog::onWidthChanged);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NewDocumentDialog::onHeightChanged);

    // Store colorLabel for updates
    connect(m_colorBtn, &QPushButton::clicked, this, [this, colorLabel]() {
        colorLabel->setText(m_bgColor.name());
    });
}

int NewDocumentDialog::imageWidth() const { return m_widthSpin->value(); }
int NewDocumentDialog::imageHeight() const { return m_heightSpin->value(); }
int NewDocumentDialog::resolution() const { return m_resolutionSpin->value(); }

void NewDocumentDialog::onPresetChanged(int index) {
    const int presets[][2] = {{0,0},{640,480},{800,600},{1024,768},{1280,720},{1920,1080},{2560,1440},{3840,2160}};
    if (index > 0 && index < 8) {
        m_updating = true;
        m_widthSpin->setValue(presets[index][0]);
        m_heightSpin->setValue(presets[index][1]);
        m_ratio = (double)presets[index][0] / presets[index][1];
        m_sizeInfoLabel->setText(QString("%1 \u00d7 %2 pixels").arg(presets[index][0]).arg(presets[index][1]));
        m_updating = false;
    }
}

void NewDocumentDialog::onChooseColor() {
    QColor c = QColorDialog::getColor(m_bgColor, this, TR("Couleur d'arrière-plan"));
    if (c.isValid()) {
        m_bgColor = c;
        m_colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #aaa;").arg(c.name()));
    }
}

void NewDocumentDialog::onWidthChanged(int value) {
    if (m_updating) return;
    m_updating = true;
    if (m_aspectRatio->isChecked())
        m_heightSpin->setValue(static_cast<int>(value / m_ratio));
    m_sizeInfoLabel->setText(QString("%1 \u00d7 %2 pixels").arg(m_widthSpin->value()).arg(m_heightSpin->value()));
    m_presetCombo->setCurrentIndex(0);
    m_updating = false;
}

void NewDocumentDialog::onHeightChanged(int value) {
    if (m_updating) return;
    m_updating = true;
    if (m_aspectRatio->isChecked())
        m_widthSpin->setValue(static_cast<int>(value * m_ratio));
    m_sizeInfoLabel->setText(QString("%1 \u00d7 %2 pixels").arg(m_widthSpin->value()).arg(m_heightSpin->value()));
    m_presetCombo->setCurrentIndex(0);
    m_updating = false;
}
