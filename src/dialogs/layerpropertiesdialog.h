#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include "core/layer.h"

class LayerPropertiesDialog : public QDialog {
    Q_OBJECT
public:
    explicit LayerPropertiesDialog(Layer *layer, QWidget *parent = nullptr);

    QString layerName() const;
    bool isVisible() const;
    BlendMode blendMode() const;
    int opacity() const;

signals:
    // Emitted whenever a control changes, so the canvas can preview live.
    void previewChanged(int opacity255, bool visible, int blendModeIndex);

private:
    void emitPreview();

private:
    QLineEdit *m_nameEdit;
    QCheckBox *m_visibleCheck;
    QComboBox *m_blendModeCombo;
    QSlider *m_opacitySlider;
    QLabel *m_opacityLabel;
};
