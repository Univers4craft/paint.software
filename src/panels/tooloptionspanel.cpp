#include "tooloptionspanel.h"
#include "tools/tool.h"
#include "tools/shapetool.h"
#include "tools/gradienttool.h"
#include "tools/linetool.h"
#include "tools/magicwandtool.h"
#include "core/layer.h"
#include "toolicons.h"
#include "i18n.h"

#include <QHBoxLayout>
#include <QMouseEvent>

namespace {
// The tool list shown in the dropdown, in paint.net's documented group order.
struct ToolEntry { const char *name; ToolType type; };
const ToolEntry kToolEntries[] = {
    {"Sélection rectangle",              ToolType::RectSelection},
    {"Lasso de sélection",               ToolType::LassoSelection},
    {"Sélection ellipse",                ToolType::EllipseSelection},
    {"Baguette magique",                 ToolType::MagicWand},
    {"Déplacer les pixels sélectionnés", ToolType::Move},
    {"Déplacer la sélection",            ToolType::MoveSelection},
    {"Zoom",                             ToolType::Zoom},
    {"Panoramique",                      ToolType::Pan},
    {"Pot de peinture",                  ToolType::Fill},
    {"Dégradé",                          ToolType::Gradient},
    {"Pinceau",                          ToolType::Brush},
    {"Gomme",                            ToolType::Eraser},
    {"Crayon",                           ToolType::Pencil},
    {"Sélecteur de couleur",             ToolType::ColorPicker},
    {"Tampon de clonage",                ToolType::CloneStamp},
    {"Recoloriage",                      ToolType::Recolor},
    {"Texte",                            ToolType::Text},
    {"Ligne / Courbe",                   ToolType::Line},
    {"Formes",                           ToolType::Shape},
};
}

ToolOptionsPanel::ToolOptionsPanel(QWidget *parent) : QWidget(parent) {
    setFixedHeight(26);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(3, 0, 3, 0);
    layout->setSpacing(5);

    // --- Tool selector dropdown (paint.net starts the row with this) ---
    m_toolCombo = new QComboBox;
    m_toolCombo->setFixedHeight(20);
    m_toolCombo->setMinimumWidth(170);
    m_toolCombo->setIconSize(QSize(16, 16));
    for (const auto &e : kToolEntries)
        m_toolCombo->addItem(ToolIcons::forTool(e.type), TR(QString::fromUtf8(e.name)));
    layout->addWidget(m_toolCombo);
    connect(m_toolCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
        if (i >= 0 && i < int(sizeof(kToolEntries) / sizeof(kToolEntries[0])))
            emit toolChangeRequested(kToolEntries[i].type);
    });

    layout->addWidget(createSep());

    // --- Brush width ---
    m_brushSizeLabel = new QLabel(TR("Largeur :"));
    m_brushSizeLabel->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_brushSizeLabel);
    m_brushSizeSpin = new QSpinBox;
    m_brushSizeSpin->setRange(1, 500);
    m_brushSizeSpin->setValue(10);
    m_brushSizeSpin->setFixedWidth(58);
    m_brushSizeSpin->setFixedHeight(20);
    m_brushSizeSpin->setToolTip(TR("Largeur du pinceau ( [ et ] pour ajuster )"));
    layout->addWidget(m_brushSizeSpin);
    connect(m_brushSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ToolOptionsPanel::onBrushSizeChanged);

    // --- Hardness ---
    m_hardnessGroup = makeSliderGroup(TR("Dureté :"), m_hardnessLabel, m_hardnessSlider,
                                      m_hardnessValue, 0, 100, 100);
    layout->addWidget(m_hardnessGroup);
    connect(m_hardnessSlider, &QSlider::valueChanged, this, &ToolOptionsPanel::onHardnessChanged);

    // --- Spacing ---
    m_spacingGroup = makeSliderGroup(TR("Espacement :"), m_spacingLabel, m_spacingSlider,
                                     m_spacingValue, 1, 100, 15);
    layout->addWidget(m_spacingGroup);
    connect(m_spacingSlider, &QSlider::valueChanged, this, &ToolOptionsPanel::onSpacingChanged);

    // --- Tolerance ---
    m_toleranceGroup = makeSliderGroup(TR("Tolérance :"), m_toleranceLabel, m_toleranceSlider,
                                       m_toleranceValue, 0, 255, 32);
    layout->addWidget(m_toleranceGroup);
    connect(m_toleranceSlider, &QSlider::valueChanged, this, &ToolOptionsPanel::onToleranceChanged);

    // --- Opacity ---
    m_opacityLabel = new QLabel(TR("Opacité :"));
    m_opacityLabel->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_opacityLabel);
    m_opacitySpin = new QSpinBox;
    m_opacitySpin->setRange(1, 100);
    m_opacitySpin->setValue(100);
    m_opacitySpin->setSuffix("%");
    m_opacitySpin->setFixedWidth(58);
    m_opacitySpin->setFixedHeight(20);
    layout->addWidget(m_opacitySpin);
    connect(m_opacitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ToolOptionsPanel::onOpacityChanged);

    // --- Fill mode (shapes) ---
    m_fillLabel = new QLabel(TR("Remplissage :"));
    m_fillLabel->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_fillLabel);
    m_fillCombo = new QComboBox;
    m_fillCombo->addItems({TR("Contour"), TR("Rempli"), TR("Contour + rempli")});
    m_fillCombo->setFixedHeight(20);
    m_fillCombo->setFixedWidth(120);
    layout->addWidget(m_fillCombo);
    connect(m_fillCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolOptionsPanel::onFillModeChanged);

    // --- Tool-specific variant (shape / gradient type / line style / flood mode) ---
    m_variantLabel = new QLabel(TR("Type :"));
    m_variantLabel->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_variantLabel);
    m_variantCombo = new QComboBox;
    m_variantCombo->setFixedHeight(20);
    m_variantCombo->setFixedWidth(130);
    layout->addWidget(m_variantCombo);
    connect(m_variantCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolOptionsPanel::onVariantChanged);

    // --- Blend mode (brush-like tools) ---
    m_blendModeLabel = new QLabel(TR("Mode :"));
    m_blendModeLabel->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_blendModeLabel);
    m_blendModeCombo = new QComboBox;
    for (auto mode : Layer::allBlendModes())
        m_blendModeCombo->addItem(Layer::blendModeName(mode));
    m_blendModeCombo->setFixedHeight(20);
    m_blendModeCombo->setFixedWidth(100);
    layout->addWidget(m_blendModeCombo);
    connect(m_blendModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolOptionsPanel::onBlendModeChanged);

    // --- Antialiasing ---
    m_antialiasCheck = new QCheckBox(TR("Anticrénelage"));
    m_antialiasCheck->setStyleSheet("font-size: 11px;");
    m_antialiasCheck->setChecked(true);
    layout->addWidget(m_antialiasCheck);
    connect(m_antialiasCheck, &QCheckBox::toggled, this, &ToolOptionsPanel::onAntialiasToggled);

    layout->addStretch();
}

QWidget *ToolOptionsPanel::makeSliderGroup(const QString &labelText, QLabel *&label,
                                           QSlider *&slider, QLabel *&valueLabel,
                                           int min, int max, int value) {
    auto *group = new QWidget(this);
    // Fixed width: otherwise the toolbar hands the group extra space and the
    // slider drifts far away from its label.
    group->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto *l = new QHBoxLayout(group);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(3);

    label = new QLabel(labelText);
    label->setStyleSheet("font-size: 11px;");
    l->addWidget(label);

    slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    slider->setFixedWidth(80);
    slider->setFixedHeight(18);
    l->addWidget(slider);

    valueLabel = new QLabel;
    valueLabel->setStyleSheet("font-size: 11px;");
    valueLabel->setFixedWidth(32);
    const bool percent = (max == 100);
    valueLabel->setText(percent ? QString("%1%").arg(value) : QString::number(value));
    l->addWidget(valueLabel);

    // Keep the readout live.
    connect(slider, &QSlider::valueChanged, valueLabel, [valueLabel, percent](int v) {
        valueLabel->setText(percent ? QString("%1%").arg(v) : QString::number(v));
    });

    return group;
}

QWidget *ToolOptionsPanel::createSep() {
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setFixedHeight(18);
    return sep;
}

void ToolOptionsPanel::setTool(Tool *tool) {
    m_tool = tool;
    updateFromTool();
}

void ToolOptionsPanel::updateFromTool() {
    if (!m_tool) return;

    // Reflect the active tool in the dropdown.
    for (int i = 0; i < int(sizeof(kToolEntries) / sizeof(kToolEntries[0])); ++i) {
        if (kToolEntries[i].type == m_tool->type()) {
            m_toolCombo->blockSignals(true);
            m_toolCombo->setCurrentIndex(i);
            m_toolCombo->blockSignals(false);
            break;
        }
    }

    m_brushSizeSpin->blockSignals(true);
    m_brushSizeSpin->setValue(m_tool->brushSize());
    m_brushSizeSpin->blockSignals(false);

    m_hardnessSlider->blockSignals(true);
    m_hardnessSlider->setValue(m_tool->hardness());
    m_hardnessSlider->blockSignals(false);
    m_hardnessValue->setText(QString("%1%").arg(m_tool->hardness()));

    m_spacingSlider->blockSignals(true);
    m_spacingSlider->setValue(m_tool->spacing());
    m_spacingSlider->blockSignals(false);
    m_spacingValue->setText(QString("%1%").arg(m_tool->spacing()));

    m_toleranceSlider->blockSignals(true);
    m_toleranceSlider->setValue(m_tool->tolerance());
    m_toleranceSlider->blockSignals(false);
    m_toleranceValue->setText(QString::number(m_tool->tolerance()));

    m_opacitySpin->blockSignals(true);
    m_opacitySpin->setValue(m_tool->opacity());
    m_opacitySpin->blockSignals(false);

    m_antialiasCheck->blockSignals(true);
    m_antialiasCheck->setChecked(m_tool->antialiased());
    m_antialiasCheck->blockSignals(false);

    m_blendModeCombo->blockSignals(true);
    m_blendModeCombo->setCurrentIndex(m_tool->blendMode());
    m_blendModeCombo->blockSignals(false);

    if (auto *shape = dynamic_cast<ShapeTool*>(m_tool)) {
        m_fillCombo->blockSignals(true);
        m_fillCombo->setCurrentIndex(static_cast<int>(shape->shapeFill()));
        m_fillCombo->blockSignals(false);
    }

    populateVariantCombo();

    // Show only the controls that apply to this tool.
    const ToolType t = m_tool->type();
    const bool isBrushLike = (t == ToolType::Brush || t == ToolType::Eraser || t == ToolType::CloneStamp);
    const bool hasTolerance = (t == ToolType::Fill || t == ToolType::MagicWand || t == ToolType::Recolor);
    const bool hasSize = isBrushLike || t == ToolType::Shape || t == ToolType::Line
                         || t == ToolType::Pencil || t == ToolType::Recolor;
    const bool hasFill = (t == ToolType::Shape);
    const bool hasOpacity = hasSize || t == ToolType::Fill || t == ToolType::Gradient
                            || t == ToolType::Text;
    const bool hasAA = hasSize || t == ToolType::Fill || t == ToolType::Gradient
                       || t == ToolType::Text;

    m_brushSizeLabel->setVisible(hasSize);
    m_brushSizeSpin->setVisible(hasSize);
    m_hardnessGroup->setVisible(isBrushLike);
    m_spacingGroup->setVisible(isBrushLike);
    m_toleranceGroup->setVisible(hasTolerance);
    m_opacityLabel->setVisible(hasOpacity);
    m_opacitySpin->setVisible(hasOpacity);
    m_fillLabel->setVisible(hasFill);
    m_fillCombo->setVisible(hasFill);
    const bool hasVariant = (t == ToolType::Shape || t == ToolType::Gradient
                             || t == ToolType::Line || t == ToolType::MagicWand);
    m_variantLabel->setVisible(hasVariant);
    m_variantCombo->setVisible(hasVariant);
    m_blendModeLabel->setVisible(t == ToolType::Brush);
    m_blendModeCombo->setVisible(t == ToolType::Brush);
    m_antialiasCheck->setVisible(hasAA);
}

void ToolOptionsPanel::onBrushSizeChanged(int value) {
    if (m_tool) { m_tool->setBrushSize(value); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::onHardnessChanged(int value) {
    if (m_tool) { m_tool->setHardness(value); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::onOpacityChanged(int value) {
    if (m_tool) { m_tool->setOpacity(value); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::onToleranceChanged(int value) {
    if (m_tool) { m_tool->setTolerance(value); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::onAntialiasToggled(bool checked) {
    if (m_tool) { m_tool->setAntialiased(checked); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::onBlendModeChanged(int index) {
    if (m_tool) { m_tool->setBlendMode(index); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::populateVariantCombo() {
    m_variantCombo->blockSignals(true);
    m_variantCombo->clear();

    if (auto *shape = dynamic_cast<ShapeTool*>(m_tool)) {
        m_variantLabel->setText(TR("Forme :"));
        m_variantCombo->addItems({TR("Rectangle"), TR("Ellipse"), TR("Rectangle arrondi"),
                                  TR("Triangle"), TR("Losange"), TR("Pentagone"),
                                  TR("Hexagone"), TR("Étoile")});
        m_variantCombo->setCurrentIndex(static_cast<int>(shape->shapeType()));
    } else if (auto *grad = dynamic_cast<GradientTool*>(m_tool)) {
        m_variantLabel->setText(TR("Type :"));
        m_variantCombo->addItems({TR("Linéaire"), TR("Radial"), TR("Conique"), TR("Losange")});
        m_variantCombo->setCurrentIndex(static_cast<int>(grad->gradientType()));
    } else if (auto *line = dynamic_cast<LineTool*>(m_tool)) {
        m_variantLabel->setText(TR("Style :"));
        m_variantCombo->addItems({TR("Trait plein"), TR("Flèche"), TR("Pointillés")});
        m_variantCombo->setCurrentIndex(static_cast<int>(line->lineStyle()));
    } else if (auto *wand = dynamic_cast<MagicWandTool*>(m_tool)) {
        m_variantLabel->setText(TR("Remplissage :"));
        m_variantCombo->addItems({TR("Contigu"), TR("Global")});
        m_variantCombo->setCurrentIndex(wand->isGlobal() ? 1 : 0);
    }

    m_variantCombo->blockSignals(false);
}

void ToolOptionsPanel::onVariantChanged(int index) {
    if (index < 0) return;
    if (auto *shape = dynamic_cast<ShapeTool*>(m_tool))
        shape->setShapeType(static_cast<ShapeType>(index));
    else if (auto *grad = dynamic_cast<GradientTool*>(m_tool))
        grad->setGradientType(static_cast<GradientType>(index));
    else if (auto *line = dynamic_cast<LineTool*>(m_tool))
        line->setLineStyle(static_cast<LineTool::LineStyle>(index));
    else if (auto *wand = dynamic_cast<MagicWandTool*>(m_tool))
        wand->setGlobal(index == 1);
    emit toolOptionsChanged();
}

void ToolOptionsPanel::onFillModeChanged(int index) {
    if (auto *shape = dynamic_cast<ShapeTool*>(m_tool))
        shape->setShapeFill(static_cast<ShapeFill>(qBound(0, index, 2)));
    emit toolOptionsChanged();
}

void ToolOptionsPanel::onSpacingChanged(int value) {
    if (m_tool) { m_tool->setSpacing(qMax(1, value)); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::retranslate() {
    m_brushSizeLabel->setText(TR("Largeur :"));
    m_hardnessLabel->setText(TR("Dureté :"));
    m_spacingLabel->setText(TR("Espacement :"));
    m_toleranceLabel->setText(TR("Tolérance :"));
    m_opacityLabel->setText(TR("Opacité :"));
    m_fillLabel->setText(TR("Remplissage :"));
    m_blendModeLabel->setText(TR("Mode :"));
    m_antialiasCheck->setText(TR("Anticrénelage"));

    const int fill = m_fillCombo->currentIndex();
    m_fillCombo->blockSignals(true);
    m_fillCombo->clear();
    m_fillCombo->addItems({TR("Contour"), TR("Rempli"), TR("Contour + rempli")});
    m_fillCombo->setCurrentIndex(fill);
    m_fillCombo->blockSignals(false);

    populateVariantCombo();

    // Rebuild the tool dropdown labels, keeping the current selection.
    const int cur = m_toolCombo->currentIndex();
    m_toolCombo->blockSignals(true);
    m_toolCombo->clear();
    for (const auto &e : kToolEntries)
        m_toolCombo->addItem(ToolIcons::forTool(e.type), TR(QString::fromUtf8(e.name)));
    m_toolCombo->setCurrentIndex(cur);
    m_toolCombo->blockSignals(false);
}
