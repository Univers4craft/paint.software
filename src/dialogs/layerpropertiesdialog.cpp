#include "layerpropertiesdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

LayerPropertiesDialog::LayerPropertiesDialog(Layer *layer, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Propriétés du calque");
    setFixedWidth(340);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // Section générale
    auto *generalGroup = new QGroupBox("Général");
    auto *generalForm = new QFormLayout(generalGroup);
    generalForm->setSpacing(6);

    m_nameEdit = new QLineEdit(layer->name());
    generalForm->addRow("Nom :", m_nameEdit);

    m_visibleCheck = new QCheckBox("Visible");
    m_visibleCheck->setChecked(layer->isVisible());
    generalForm->addRow(m_visibleCheck);

    layout->addWidget(generalGroup);

    // Section mode de fusion
    auto *blendGroup = new QGroupBox("Mode de fusion");
    auto *blendLayout = new QVBoxLayout(blendGroup);

    m_blendModeCombo = new QComboBox;
    for (auto mode : Layer::allBlendModes())
        m_blendModeCombo->addItem(Layer::blendModeName(mode));
    m_blendModeCombo->setCurrentIndex(static_cast<int>(layer->blendMode()));
    blendLayout->addWidget(m_blendModeCombo);

    layout->addWidget(blendGroup);

    // Section opacité
    auto *opacityGroup = new QGroupBox("Opacité");
    auto *opacityLayout = new QHBoxLayout(opacityGroup);

    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 255);
    m_opacitySlider->setValue(static_cast<int>(layer->opacity() * 255));
    opacityLayout->addWidget(m_opacitySlider);

    m_opacityLabel = new QLabel(QString::number(static_cast<int>(layer->opacity() * 255)));
    m_opacityLabel->setFixedWidth(30);
    m_opacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    opacityLayout->addWidget(m_opacityLabel);

    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int v) {
        m_opacityLabel->setText(QString::number(v));
        emitPreview();
    });

    layout->addWidget(opacityGroup);

    // Live preview: any change updates the canvas immediately.
    connect(m_visibleCheck, &QCheckBox::toggled, this, [this]() { emitPreview(); });
    connect(m_blendModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { emitPreview(); });

    // OK / Annuler
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    m_nameEdit->selectAll();
    m_nameEdit->setFocus();
}

void LayerPropertiesDialog::emitPreview() {
    emit previewChanged(m_opacitySlider->value(),
                        m_visibleCheck->isChecked(),
                        m_blendModeCombo->currentIndex());
}

QString LayerPropertiesDialog::layerName() const {
    return m_nameEdit->text();
}

bool LayerPropertiesDialog::isVisible() const {
    return m_visibleCheck->isChecked();
}

BlendMode LayerPropertiesDialog::blendMode() const {
    auto modes = Layer::allBlendModes();
    int idx = m_blendModeCombo->currentIndex();
    if (idx >= 0 && idx < modes.size())
        return modes[idx];
    return BlendMode::Normal;
}

int LayerPropertiesDialog::opacity() const {
    return m_opacitySlider->value();
}
