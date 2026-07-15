#pragma once
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QColor>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

class NewDocumentDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewDocumentDialog(QWidget *parent = nullptr);
    int imageWidth() const;
    int imageHeight() const;
    int resolution() const;
    QColor backgroundColor() const { return m_bgColor; }
private slots:
    void onPresetChanged(int index);
    void onChooseColor();
    void onWidthChanged(int value);
    void onHeightChanged(int value);
private:
    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
    QSpinBox *m_resolutionSpin;
    QComboBox *m_presetCombo;
    QCheckBox *m_aspectRatio;
    QPushButton *m_colorBtn;
    QLabel *m_sizeInfoLabel;
    QColor m_bgColor = Qt::white;
    double m_ratio;
    bool m_updating = false;
};
