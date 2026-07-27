#include "tooloptionspanel.h"
#include "tools/tool.h"
#include "tools/shapetool.h"
#include "tools/gradienttool.h"
#include "tools/linetool.h"
#include "tools/magicwandtool.h"
#include "tools/filltool.h"
#include "tools/texttool.h"
#include "core/hatchpatterns.h"
#include "core/layer.h"
#include "toolicons.h"
#include "i18n.h"
#include <QLineEdit>
#include <QDoubleValidator>

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
    // Editable combo: type any size (decimals allowed) or pick a preset, like
    // Paint.NET. The presets march 1..2000 with widening steps.
    m_brushSizeCombo = new QComboBox;
    m_brushSizeCombo->setEditable(true);
    m_brushSizeCombo->setInsertPolicy(QComboBox::NoInsert);
    for (int p : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22,
                  25, 28, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90, 100, 125, 150, 175,
                  200, 225, 250, 275, 300, 350, 400, 450, 500, 600, 700, 800, 900,
                  1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000})
        m_brushSizeCombo->addItem(QString::number(p));
    auto *validator = new QDoubleValidator(1.0, 2000.0, 2, m_brushSizeCombo);
    validator->setNotation(QDoubleValidator::StandardNotation);
    m_brushSizeCombo->setValidator(validator);
    m_brushSizeCombo->setCurrentText("10");
    m_brushSizeCombo->setFixedWidth(62);
    m_brushSizeCombo->setFixedHeight(20);
    m_brushSizeCombo->setToolTip(TR("Largeur du pinceau ( [ et ] pour ajuster )"));
    layout->addWidget(m_brushSizeCombo);
    // Commit on Enter / focus-out and on picking a preset — not per keystroke,
    // so typing "6.5" isn't cut short (same reasoning as the resize dialog).
    connect(m_brushSizeCombo, QOverload<int>::of(&QComboBox::activated),
            this, &ToolOptionsPanel::onBrushSizeChanged);
    connect(m_brushSizeCombo->lineEdit(), &QLineEdit::editingFinished,
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
                                       m_toleranceValue, 0, 100, 50);
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
    m_opacitySpin->setKeyboardTracking(false);   // commit on Enter/focus-out, not per digit
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

    // --- Fill Style: solid colour + GDI+ hatch patterns (Paint.NET) ---
    m_fillStyleLabel = new QLabel(TR("Style de remplissage :"));
    m_fillStyleLabel->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_fillStyleLabel);
    m_fillStyleCombo = new QComboBox;
    for (int i = 0; i < Hatch::count(); ++i)
        m_fillStyleCombo->addItem(TR(Hatch::name(i)));
    m_fillStyleCombo->setFixedHeight(20);
    m_fillStyleCombo->setFixedWidth(150);
    m_fillStyleCombo->setToolTip(TR("Motif de remplissage"));
    layout->addWidget(m_fillStyleCombo);
    connect(m_fillStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolOptionsPanel::onFillStyleChanged);

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

    // --- Text tool: font family, size, and B / I / U ---
    m_fontCombo = new QFontComboBox;
    m_fontCombo->setFixedHeight(20);
    m_fontCombo->setFixedWidth(150);
    m_fontCombo->setToolTip(TR("Police"));
    layout->addWidget(m_fontCombo);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &ToolOptionsPanel::onFontChanged);

    m_fontSizeLabel = new QLabel(TR("Taille :"));
    m_fontSizeLabel->setStyleSheet("font-size: 11px;");
    layout->addWidget(m_fontSizeLabel);
    m_fontSizeSpin = new QSpinBox;
    m_fontSizeSpin->setRange(1, 500);
    m_fontSizeSpin->setValue(24);
    m_fontSizeSpin->setKeyboardTracking(false);   // commit on Enter/focus-out, not per digit
    m_fontSizeSpin->setFixedWidth(52);
    m_fontSizeSpin->setFixedHeight(20);
    layout->addWidget(m_fontSizeSpin);
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ToolOptionsPanel::onFontSizeChanged);

    auto makeStyleButton = [this](const QString &text, const QString &tip, bool italic, bool underline) {
        auto *b = new QToolButton;
        b->setCheckable(true);
        b->setText(text);
        b->setToolTip(tip);
        b->setFixedSize(22, 20);
        QFont f = b->font();
        f.setBold(true);
        f.setItalic(italic);
        f.setUnderline(underline);
        b->setFont(f);
        connect(b, &QToolButton::toggled, this, &ToolOptionsPanel::onTextStyleChanged);
        return b;
    };
    m_boldBtn = makeStyleButton("B", TR("Gras"), false, false);
    m_italicBtn = makeStyleButton("I", TR("Italique"), true, false);
    m_underlineBtn = makeStyleButton("U", TR("Souligné"), false, true);
    m_strikeBtn = makeStyleButton("S", TR("Barré"), false, false);
    { QFont f = m_strikeBtn->font(); f.setStrikeOut(true); m_strikeBtn->setFont(f); }
    layout->addWidget(m_boldBtn);
    layout->addWidget(m_italicBtn);
    layout->addWidget(m_underlineBtn);
    layout->addWidget(m_strikeBtn);

    m_alignCombo = new QComboBox;
    m_alignCombo->addItems({TR("Gauche"), TR("Centré"), TR("Droite")});
    m_alignCombo->setFixedHeight(20);
    m_alignCombo->setToolTip(TR("Alignement du texte"));
    layout->addWidget(m_alignCombo);
    connect(m_alignCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolOptionsPanel::onTextStyleChanged);

    // --- Antialiasing ---
    m_antialiasCheck = new QCheckBox(TR("Anticrénelage"));
    m_antialiasCheck->setStyleSheet("font-size: 11px;");
    m_antialiasCheck->setChecked(true);
    layout->addWidget(m_antialiasCheck);
    connect(m_antialiasCheck, &QCheckBox::toggled, this, &ToolOptionsPanel::onAntialiasToggled);

    m_pressureCheck = new QCheckBox(TR("Pression"));
    m_pressureCheck->setStyleSheet("font-size: 11px;");
    m_pressureCheck->setChecked(true);
    m_pressureCheck->setToolTip(TR("Sensibilité à la pression du stylet (varie la taille du point)"));
    // Paint.NET puts the Pressure toggle right after Brush size (before Hardness).
    layout->insertWidget(layout->indexOf(m_brushSizeCombo) + 1, m_pressureCheck);
    connect(m_pressureCheck, &QCheckBox::toggled, this, &ToolOptionsPanel::onPressureToggled);

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

    showBrushSize(m_tool->brushSize());

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

    m_pressureCheck->blockSignals(true);
    m_pressureCheck->setChecked(m_tool->pressureSensitivity());
    m_pressureCheck->blockSignals(false);

    m_blendModeCombo->blockSignals(true);
    m_blendModeCombo->setCurrentIndex(m_tool->blendMode());
    m_blendModeCombo->blockSignals(false);

    if (auto *shape = dynamic_cast<ShapeTool*>(m_tool)) {
        m_fillCombo->blockSignals(true);
        m_fillCombo->setCurrentIndex(static_cast<int>(shape->shapeFill()));
        m_fillCombo->blockSignals(false);
    }

    m_fillStyleCombo->blockSignals(true);
    m_fillStyleCombo->setCurrentIndex(qBound(0, m_tool->fillStyle(), m_fillStyleCombo->count() - 1));
    m_fillStyleCombo->blockSignals(false);

    if (auto *text = dynamic_cast<TextTool*>(m_tool)) {
        m_fontCombo->blockSignals(true);
        m_fontCombo->setCurrentFont(QFont(text->fontFamily()));
        m_fontCombo->blockSignals(false);
        m_fontSizeSpin->blockSignals(true);
        m_fontSizeSpin->setValue(text->fontSize());
        m_fontSizeSpin->blockSignals(false);
        m_boldBtn->blockSignals(true);      m_boldBtn->setChecked(text->bold());           m_boldBtn->blockSignals(false);
        m_italicBtn->blockSignals(true);    m_italicBtn->setChecked(text->italic());       m_italicBtn->blockSignals(false);
        m_underlineBtn->blockSignals(true); m_underlineBtn->setChecked(text->underline()); m_underlineBtn->blockSignals(false);
        m_strikeBtn->blockSignals(true);    m_strikeBtn->setChecked(text->strikeOut());    m_strikeBtn->blockSignals(false);
        m_alignCombo->blockSignals(true);   m_alignCombo->setCurrentIndex(static_cast<int>(text->alignment())); m_alignCombo->blockSignals(false);
    }

    populateVariantCombo();

    // Show only the controls that apply to this tool.
    const ToolType t = m_tool->type();
    const bool isBrushLike = (t == ToolType::Brush || t == ToolType::Eraser || t == ToolType::CloneStamp);
    const bool hasTolerance = (t == ToolType::Fill || t == ToolType::MagicWand || t == ToolType::Recolor);
    // Pencil is always one pixel in Paint.NET, so it gets no width control.
    const bool hasSize = isBrushLike || t == ToolType::Shape || t == ToolType::Line
                         || t == ToolType::Recolor;
    const bool hasFill = (t == ToolType::Shape);
    // Fill Style (solid/hatch) applies to the tools that lay down filled colour.
    const bool hasFillStyle = (t == ToolType::Brush || t == ToolType::Shape);
    const bool hasOpacity = hasSize || t == ToolType::Fill || t == ToolType::Gradient
                            || t == ToolType::Text;
    const bool hasAA = hasSize || t == ToolType::Fill || t == ToolType::Gradient
                       || t == ToolType::Text;

    m_brushSizeLabel->setVisible(hasSize);
    m_brushSizeCombo->setVisible(hasSize);
    m_hardnessGroup->setVisible(isBrushLike);
    m_spacingGroup->setVisible(isBrushLike);
    m_toleranceGroup->setVisible(hasTolerance);
    m_opacityLabel->setVisible(hasOpacity);
    m_opacitySpin->setVisible(hasOpacity);
    m_fillLabel->setVisible(hasFill);
    m_fillCombo->setVisible(hasFill);
    m_fillStyleLabel->setVisible(hasFillStyle);
    m_fillStyleCombo->setVisible(hasFillStyle);
    const bool hasVariant = (t == ToolType::Shape || t == ToolType::Gradient
                             || t == ToolType::Line || t == ToolType::MagicWand
                             || t == ToolType::Fill);
    m_variantLabel->setVisible(hasVariant);
    m_variantCombo->setVisible(hasVariant);
    m_blendModeLabel->setVisible(t == ToolType::Brush);
    m_blendModeCombo->setVisible(t == ToolType::Brush);

    const bool hasText = (t == ToolType::Text);
    m_fontCombo->setVisible(hasText);
    m_fontSizeLabel->setVisible(hasText);
    m_fontSizeSpin->setVisible(hasText);
    m_boldBtn->setVisible(hasText);
    m_italicBtn->setVisible(hasText);
    m_underlineBtn->setVisible(hasText);
    m_strikeBtn->setVisible(hasText);
    m_alignCombo->setVisible(hasText);

    m_antialiasCheck->setVisible(hasAA);
    // Shown only when a tablet has been detected (Paint.NET), on brush-like tools.
    m_pressureCheck->setVisible(isBrushLike && m_tabletPresent);
}

void ToolOptionsPanel::onBrushSizeChanged() {
    if (!m_tool) return;
    bool ok = false;
    const double v = m_brushSizeCombo->currentText().toDouble(&ok);
    if (!ok) {   // reject garbage: snap the field back to the tool's real size
        showBrushSize(m_tool->brushSize());
        return;
    }
    m_tool->setBrushSize(v);
    // Reflect any clamping (1..2000) and drop trailing zeros.
    showBrushSize(m_tool->brushSize());
    emit toolOptionsChanged();
}

// Renders a brush size as a compact string: "10", not "10.00"; "6.5" kept.
void ToolOptionsPanel::showBrushSize(double size) {
    QString text = QString::number(size, 'f', 2);
    if (text.contains('.')) {
        while (text.endsWith('0')) text.chop(1);
        if (text.endsWith('.')) text.chop(1);
    }
    m_brushSizeCombo->blockSignals(true);
    m_brushSizeCombo->setCurrentText(text);
    m_brushSizeCombo->blockSignals(false);
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

void ToolOptionsPanel::onPressureToggled(bool checked) {
    if (m_tool) { m_tool->setPressureSensitivity(checked); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::setTabletPresent(bool present) {
    if (m_tabletPresent == present) return;
    m_tabletPresent = present;
    if (m_tool) setTool(m_tool);   // refresh which controls are visible
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
    } else if (auto *fill = dynamic_cast<FillTool*>(m_tool)) {
        m_variantLabel->setText(TR("Remplissage :"));
        m_variantCombo->addItems({TR("Contigu"), TR("Global")});
        m_variantCombo->setCurrentIndex(fill->isGlobal() ? 1 : 0);
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
    else if (auto *fill = dynamic_cast<FillTool*>(m_tool))
        fill->setGlobal(index == 1);
    emit toolOptionsChanged();
}

void ToolOptionsPanel::onFillStyleChanged(int index) {
    if (m_tool && index >= 0) { m_tool->setFillStyle(index); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::onFillModeChanged(int index) {
    if (auto *shape = dynamic_cast<ShapeTool*>(m_tool))
        shape->setShapeFill(static_cast<ShapeFill>(qBound(0, index, 2)));
    emit toolOptionsChanged();
}

void ToolOptionsPanel::onSpacingChanged(int value) {
    if (m_tool) { m_tool->setSpacing(qMax(1, value)); emit toolOptionsChanged(); }
}

void ToolOptionsPanel::onFontChanged(const QFont &font) {
    if (auto *text = dynamic_cast<TextTool*>(m_tool)) {
        text->setFontFamily(font.family());
        emit toolOptionsChanged();
    }
}

void ToolOptionsPanel::onFontSizeChanged(int value) {
    if (auto *text = dynamic_cast<TextTool*>(m_tool)) {
        text->setFontSize(value);
        emit toolOptionsChanged();
    }
}

void ToolOptionsPanel::onTextStyleChanged() {
    if (auto *text = dynamic_cast<TextTool*>(m_tool)) {
        text->setBold(m_boldBtn->isChecked());
        text->setItalic(m_italicBtn->isChecked());
        text->setUnderline(m_underlineBtn->isChecked());
        text->setStrikeOut(m_strikeBtn->isChecked());
        text->setAlignment(static_cast<TextTool::Align>(m_alignCombo->currentIndex()));
        emit toolOptionsChanged();
    }
}

void ToolOptionsPanel::retranslate() {
    m_brushSizeLabel->setText(TR("Largeur :"));
    m_hardnessLabel->setText(TR("Dureté :"));
    m_spacingLabel->setText(TR("Espacement :"));
    m_toleranceLabel->setText(TR("Tolérance :"));
    m_opacityLabel->setText(TR("Opacité :"));
    m_fillLabel->setText(TR("Remplissage :"));
    m_fillStyleLabel->setText(TR("Style de remplissage :"));
    m_fillStyleCombo->setToolTip(TR("Motif de remplissage"));
    m_blendModeLabel->setText(TR("Mode :"));
    m_fontSizeLabel->setText(TR("Taille :"));
    m_fontCombo->setToolTip(TR("Police"));
    m_boldBtn->setToolTip(TR("Gras"));
    m_italicBtn->setToolTip(TR("Italique"));
    m_underlineBtn->setToolTip(TR("Souligné"));
    m_strikeBtn->setToolTip(TR("Barré"));
    m_alignCombo->setToolTip(TR("Alignement du texte"));
    {
        const int a = m_alignCombo->currentIndex();
        m_alignCombo->blockSignals(true);
        m_alignCombo->clear();
        m_alignCombo->addItems({TR("Gauche"), TR("Centré"), TR("Droite")});
        m_alignCombo->setCurrentIndex(a < 0 ? 0 : a);
        m_alignCombo->blockSignals(false);
    }
    m_antialiasCheck->setText(TR("Anticrénelage"));
    m_pressureCheck->setText(TR("Pression"));
    m_pressureCheck->setToolTip(TR("Sensibilité à la pression du stylet (varie la taille du point)"));

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
