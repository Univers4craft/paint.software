#pragma once
#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>

class ResizeDialog : public QDialog {
    Q_OBJECT
public:
    explicit ResizeDialog(int currentWidth, int currentHeight, int dpi = 96, QWidget *parent = nullptr);

    int newWidth() const;
    int newHeight() const;
    int resampleMethod() const;
    int resolution() const;

private slots:
    void onPixelWidthChanged(int value);
    void onPixelHeightChanged(int value);
    void onPercentChanged(double value);
    void onPrintWidthChanged(double value);
    void onPrintHeightChanged(double value);
    void onResolutionChanged(int value);
    void onModeChanged();

private:
    void updateNewSizeLabel();
    void syncFromPixels();

    // Pixel size
    QRadioButton *m_absoluteRB;
    QSpinBox *m_pixelWidthSpin;
    QSpinBox *m_pixelHeightSpin;

    // Percentage
    QRadioButton *m_percentRB;
    QDoubleSpinBox *m_percentSpin;

    // Print size
    QDoubleSpinBox *m_printWidthSpin;
    QDoubleSpinBox *m_printHeightSpin;

    // Resolution
    QSpinBox *m_resolutionSpin;

    // Common
    QCheckBox *m_aspectRatio;
    QComboBox *m_resampleCombo;
    QLabel *m_newSizeLabel;

    int m_origWidth;
    int m_origHeight;
    double m_ratio;
    bool m_updating = false;
};
