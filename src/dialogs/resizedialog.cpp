#include "resizedialog.h"
#include "../i18n.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QButtonGroup>

ResizeDialog::ResizeDialog(int currentWidth, int currentHeight, int dpi, QWidget *parent)
    : QDialog(parent), m_origWidth(currentWidth), m_origHeight(currentHeight)
{
    setWindowTitle(TR("Redimensionner"));
    setFixedWidth(380);
    m_ratio = (double)currentWidth / currentHeight;

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // -- Pixel Size section --
    auto *pixelGroup = new QGroupBox(TR("Taille en pixels"));
    auto *pixelGrid = new QGridLayout(pixelGroup);
    pixelGrid->setSpacing(4);

    m_absoluteRB = new QRadioButton(TR("Par taille &absolue :"));
    m_absoluteRB->setChecked(true);
    pixelGrid->addWidget(m_absoluteRB, 0, 0, 1, 3);

    pixelGrid->addWidget(new QLabel(TR("Largeur :")), 1, 0);
    m_pixelWidthSpin = new QSpinBox;
    m_pixelWidthSpin->setRange(1, 65535);
    m_pixelWidthSpin->setValue(currentWidth);
    pixelGrid->addWidget(m_pixelWidthSpin, 1, 1);
    pixelGrid->addWidget(new QLabel(TR("pixels")), 1, 2);

    pixelGrid->addWidget(new QLabel(TR("Hauteur :")), 2, 0);
    m_pixelHeightSpin = new QSpinBox;
    m_pixelHeightSpin->setRange(1, 65535);
    m_pixelHeightSpin->setValue(currentHeight);
    pixelGrid->addWidget(m_pixelHeightSpin, 2, 1);
    pixelGrid->addWidget(new QLabel(TR("pixels")), 2, 2);

    m_percentRB = new QRadioButton(TR("Par &pourcentage :"));
    pixelGrid->addWidget(m_percentRB, 3, 0, 1, 2);

    m_percentSpin = new QDoubleSpinBox;
    m_percentSpin->setRange(0.01, 10000.0);
    m_percentSpin->setValue(100.0);
    m_percentSpin->setSuffix(" %");
    m_percentSpin->setEnabled(false);
    pixelGrid->addWidget(m_percentSpin, 3, 1);

    auto *modeGroup = new QButtonGroup(this);
    modeGroup->addButton(m_absoluteRB, 0);
    modeGroup->addButton(m_percentRB, 1);

    layout->addWidget(pixelGroup);

    // -- Print Size section --
    auto *printGroup = new QGroupBox(TR("Taille d'impression"));
    auto *printGrid = new QGridLayout(printGroup);
    printGrid->setSpacing(4);

    printGrid->addWidget(new QLabel(TR("Largeur :")), 0, 0);
    m_printWidthSpin = new QDoubleSpinBox;
    m_printWidthSpin->setRange(0.001, 9999.0);
    m_printWidthSpin->setDecimals(3);
    m_printWidthSpin->setSuffix(" in");
    m_printWidthSpin->setValue((double)currentWidth / dpi);
    printGrid->addWidget(m_printWidthSpin, 0, 1);

    printGrid->addWidget(new QLabel(TR("Hauteur :")), 1, 0);
    m_printHeightSpin = new QDoubleSpinBox;
    m_printHeightSpin->setRange(0.001, 9999.0);
    m_printHeightSpin->setDecimals(3);
    m_printHeightSpin->setSuffix(" in");
    m_printHeightSpin->setValue((double)currentHeight / dpi);
    printGrid->addWidget(m_printHeightSpin, 1, 1);

    printGrid->addWidget(new QLabel(TR("Résolution :")), 2, 0);
    m_resolutionSpin = new QSpinBox;
    m_resolutionSpin->setRange(1, 9999);
    m_resolutionSpin->setValue(dpi);
    m_resolutionSpin->setSuffix(" dpi");
    printGrid->addWidget(m_resolutionSpin, 2, 1);

    layout->addWidget(printGroup);

    // -- Maintain aspect ratio --
    m_aspectRatio = new QCheckBox(TR("&Conserver les proportions"));
    m_aspectRatio->setChecked(true);
    layout->addWidget(m_aspectRatio);

    // -- Resampling --
    auto *resampleLayout = new QHBoxLayout;
    resampleLayout->addWidget(new QLabel(TR("Rééchantillonnage :")));
    m_resampleCombo = new QComboBox;
    m_resampleCombo->addItem(TR("Meilleure qualité"));
    m_resampleCombo->addItem(TR("Bicubique"));
    m_resampleCombo->addItem(TR("Bilinéaire"));
    m_resampleCombo->addItem("Fant");
    m_resampleCombo->addItem(TR("Plus proche voisin"));
    m_resampleCombo->addItem(TR("Suréchantillonnage"));
    resampleLayout->addWidget(m_resampleCombo);
    layout->addLayout(resampleLayout);

    // -- New size info --
    m_newSizeLabel = new QLabel;
    m_newSizeLabel->setStyleSheet("color: #555; font-style: italic;");
    layout->addWidget(m_newSizeLabel);
    updateNewSizeLabel();

    // -- Buttons --
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    // Connections
    connect(m_pixelWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeDialog::onPixelWidthChanged);
    connect(m_pixelHeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeDialog::onPixelHeightChanged);
    connect(m_percentSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeDialog::onPercentChanged);
    connect(m_printWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeDialog::onPrintWidthChanged);
    connect(m_printHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeDialog::onPrintHeightChanged);
    connect(m_resolutionSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeDialog::onResolutionChanged);
    connect(modeGroup, &QButtonGroup::idClicked, this, [this](int) { onModeChanged(); });
}

int ResizeDialog::newWidth() const { return m_pixelWidthSpin->value(); }
int ResizeDialog::newHeight() const { return m_pixelHeightSpin->value(); }
int ResizeDialog::resampleMethod() const { return m_resampleCombo->currentIndex(); }
int ResizeDialog::resolution() const { return m_resolutionSpin->value(); }

void ResizeDialog::onModeChanged() {
    bool absolute = m_absoluteRB->isChecked();
    m_pixelWidthSpin->setEnabled(absolute);
    m_pixelHeightSpin->setEnabled(absolute);
    m_percentSpin->setEnabled(!absolute);
}

void ResizeDialog::onPixelWidthChanged(int value) {
    if (m_updating) return;
    m_updating = true;
    if (m_aspectRatio->isChecked())
        m_pixelHeightSpin->setValue(static_cast<int>(value / m_ratio));
    syncFromPixels();
    m_updating = false;
}

void ResizeDialog::onPixelHeightChanged(int value) {
    if (m_updating) return;
    m_updating = true;
    if (m_aspectRatio->isChecked())
        m_pixelWidthSpin->setValue(static_cast<int>(value * m_ratio));
    syncFromPixels();
    m_updating = false;
}

void ResizeDialog::onPercentChanged(double value) {
    if (m_updating) return;
    m_updating = true;
    int w = static_cast<int>(m_origWidth * value / 100.0);
    int h = static_cast<int>(m_origHeight * value / 100.0);
    m_pixelWidthSpin->setValue(qMax(1, w));
    m_pixelHeightSpin->setValue(qMax(1, h));
    syncFromPixels();
    m_updating = false;
}

void ResizeDialog::onPrintWidthChanged(double value) {
    if (m_updating) return;
    m_updating = true;
    int px = static_cast<int>(value * m_resolutionSpin->value());
    m_pixelWidthSpin->setValue(qMax(1, px));
    if (m_aspectRatio->isChecked())
        m_pixelHeightSpin->setValue(static_cast<int>(px / m_ratio));
    syncFromPixels();
    m_updating = false;
}

void ResizeDialog::onPrintHeightChanged(double value) {
    if (m_updating) return;
    m_updating = true;
    int px = static_cast<int>(value * m_resolutionSpin->value());
    m_pixelHeightSpin->setValue(qMax(1, px));
    if (m_aspectRatio->isChecked())
        m_pixelWidthSpin->setValue(static_cast<int>(px * m_ratio));
    syncFromPixels();
    m_updating = false;
}

void ResizeDialog::onResolutionChanged(int value) {
    if (m_updating) return;
    m_updating = true;
    m_printWidthSpin->setValue((double)m_pixelWidthSpin->value() / value);
    m_printHeightSpin->setValue((double)m_pixelHeightSpin->value() / value);
    m_updating = false;
}

void ResizeDialog::syncFromPixels() {
    int dpi = m_resolutionSpin->value();
    m_printWidthSpin->blockSignals(true);
    m_printHeightSpin->blockSignals(true);
    m_percentSpin->blockSignals(true);
    m_printWidthSpin->setValue((double)m_pixelWidthSpin->value() / dpi);
    m_printHeightSpin->setValue((double)m_pixelHeightSpin->value() / dpi);
    m_percentSpin->setValue((double)m_pixelWidthSpin->value() / m_origWidth * 100.0);
    m_printWidthSpin->blockSignals(false);
    m_printHeightSpin->blockSignals(false);
    m_percentSpin->blockSignals(false);
    updateNewSizeLabel();
}

void ResizeDialog::updateNewSizeLabel() {
    m_newSizeLabel->setText(QString(TR("Nouvelle taille : %1 x %2 pixels"))
        .arg(m_pixelWidthSpin->value()).arg(m_pixelHeightSpin->value()));
}
