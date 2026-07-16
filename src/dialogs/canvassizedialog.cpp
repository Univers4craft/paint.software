#include "canvassizedialog.h"
#include "../i18n.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>

CanvasSizeDialog::CanvasSizeDialog(int currentWidth, int currentHeight, QWidget *parent)
    : QDialog(parent), m_currentWidth(currentWidth), m_currentHeight(currentHeight)
{
    setWindowTitle(TR("Taille du canevas"));
    setFixedWidth(340);
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // Size section
    auto *sizeGroup = new QGroupBox(TR("Nouvelle taille"));
    auto *form = new QFormLayout(sizeGroup);
    m_widthSpin = new QSpinBox;
    m_widthSpin->setRange(1, 65535);
    m_widthSpin->setValue(currentWidth);
    m_widthSpin->setSuffix(" pixels");
    form->addRow(TR("Largeur :"), m_widthSpin);

    m_heightSpin = new QSpinBox;
    m_heightSpin->setRange(1, 65535);
    m_heightSpin->setValue(currentHeight);
    m_heightSpin->setSuffix(" pixels");
    form->addRow(TR("Hauteur :"), m_heightSpin);
    layout->addWidget(sizeGroup);

    // Anchor 3x3 grid (like Paint.NET)
    auto *anchorGroup = new QGroupBox(TR("Ancrage"));
    auto *anchorLayout = new QVBoxLayout(anchorGroup);

    auto *anchorGrid = new QGridLayout;
    anchorGrid->setSpacing(2);
    m_anchorGroup = new QButtonGroup(this);

    const char* arrows[] = {
        "\xe2\x86\x96", "\xe2\x86\x91", "\xe2\x86\x97",  // ↖ ↑ ↗
        "\xe2\x86\x90", "\xe2\x97\x8f", "\xe2\x86\x92",  // ← ● →
        "\xe2\x86\x99", "\xe2\x86\x93", "\xe2\x86\x98"   // ↙ ↓ ↘
    };

    for (int i = 0; i < 9; ++i) {
        m_anchorButtons[i] = new QToolButton;
        m_anchorButtons[i]->setText(QString::fromUtf8(arrows[i]));
        m_anchorButtons[i]->setFixedSize(32, 32);
        m_anchorButtons[i]->setCheckable(true);
        m_anchorButtons[i]->setStyleSheet(
            "QToolButton { border: 1px solid #aaa; background: #f0f0f0; font-size: 14px; }"
            "QToolButton:checked { background: #4a90d9; color: white; border: 1px solid #2a5a9a; }"
            "QToolButton:hover { background: #d0e0f0; }"
        );
        m_anchorGroup->addButton(m_anchorButtons[i], i);
        anchorGrid->addWidget(m_anchorButtons[i], i / 3, i % 3);
    }
    m_anchorButtons[0]->setChecked(true); // Top Left default
    anchorLayout->addLayout(anchorGrid);
    layout->addWidget(anchorGroup);

    // Info label
    auto *infoLabel = new QLabel(TR("Le nouvel espace sera rempli avec la couleur secondaire."));
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #555; font-size: 11px; font-style: italic;");
    layout->addWidget(infoLabel);

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_anchorGroup, &QButtonGroup::idClicked, this, &CanvasSizeDialog::onAnchorClicked);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { updateAnchor(); });
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { updateAnchor(); });
}

int CanvasSizeDialog::newWidth() const { return m_widthSpin->value(); }
int CanvasSizeDialog::newHeight() const { return m_heightSpin->value(); }

void CanvasSizeDialog::onAnchorClicked(int id) {
    m_anchorIndex = id;
    updateAnchor();
}

void CanvasSizeDialog::updateAnchor() {
    int col = m_anchorIndex % 3;
    int row = m_anchorIndex / 3;
    int dw = m_widthSpin->value() - m_currentWidth;
    int dh = m_heightSpin->value() - m_currentHeight;
    m_anchorX = (col == 0) ? 0 : (col == 1) ? dw / 2 : dw;
    m_anchorY = (row == 0) ? 0 : (row == 1) ? dh / 2 : dh;
}
