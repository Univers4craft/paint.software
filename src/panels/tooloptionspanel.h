#pragma once

#include <QWidget>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QToolButton>
#include <QLabel>
#include <QFrame>
#include <QEvent>

#include "tools/tool.h"

class Tool;

// paint.net's second toolbar row: a tool selector dropdown followed by the
// controls that configure the active tool.
class ToolOptionsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ToolOptionsPanel(QWidget *parent = nullptr);

    void setTool(Tool *tool);
    // Re-applies all visible labels in the current language.
    void retranslate();

signals:
    void toolOptionsChanged();
    void toolChangeRequested(ToolType type);

private slots:
    void onBrushSizeChanged();
    void showBrushSize(double size);   // display helper (compact, no trailing zeros)
    void onHardnessChanged(int value);
    void onOpacityChanged(int value);
    void onToleranceChanged(int value);
    void onAntialiasToggled(bool checked);
    void onBlendModeChanged(int index);
    void onFillModeChanged(int index);
    void onSpacingChanged(int value);
    // Tool-specific variant: shape type / gradient type / line style / flood mode.
    void onVariantChanged(int index);
    // Text tool.
    void onFontChanged(const QFont &font);
    void onFontSizeChanged(int value);
    void onTextStyleChanged();

private:
    void updateFromTool();
    void populateVariantCombo();
    QWidget *createSep();
    // A labelled slider with a live percentage readout.
    QWidget *makeSliderGroup(const QString &labelText, QLabel *&label, QSlider *&slider,
                             QLabel *&valueLabel, int min, int max, int value);

    Tool *m_tool = nullptr;

    QComboBox *m_toolCombo;

    QLabel *m_brushSizeLabel;
    QComboBox *m_brushSizeCombo;   // editable: type a size or pick a preset (Paint.NET style)

    QLabel *m_hardnessLabel;
    QSlider *m_hardnessSlider;
    QLabel *m_hardnessValue;
    QWidget *m_hardnessGroup;

    QLabel *m_spacingLabel;
    QSlider *m_spacingSlider;
    QLabel *m_spacingValue;
    QWidget *m_spacingGroup;

    QLabel *m_opacityLabel;
    QSpinBox *m_opacitySpin;

    QLabel *m_toleranceLabel;
    QSlider *m_toleranceSlider;
    QLabel *m_toleranceValue;
    QWidget *m_toleranceGroup;

    QCheckBox *m_antialiasCheck;

    QLabel *m_fillLabel;
    QComboBox *m_fillCombo;

    // Tool-specific variant: shape type / gradient type / line style / flood mode.
    QLabel *m_variantLabel;
    QComboBox *m_variantCombo;

    QLabel *m_blendModeLabel;
    QComboBox *m_blendModeCombo;

    // Text tool controls.
    QFontComboBox *m_fontCombo;
    QLabel *m_fontSizeLabel;
    QSpinBox *m_fontSizeSpin;
    QToolButton *m_boldBtn;
    QToolButton *m_italicBtn;
    QToolButton *m_underlineBtn;
};
