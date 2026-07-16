#pragma once

#include <QWidget>
#include <QColor>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>

class Document;

class ColorWheelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ColorWheelWidget(QWidget *parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

    // Without this the widget has no size hint, so the panel's own hint leaves
    // the disc out of its height budget and the wheel gets squeezed small.
    QSize sizeHint() const override { return QSize(190, 190); }

signals:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateFromPos(const QPoint &pos);
    void rebuildWheel();
    void pickAt(const QPoint &pos);

    QColor m_color = Qt::black;
    QImage m_wheelImage;
    QImage m_svSquare;
    int m_wheelRadius = 0;
    int m_innerRadius = 0;
    bool m_draggingWheel = false;
    bool m_draggingSquare = false;
    double m_hue = 0;
    double m_sat = 1;
    double m_val = 0;
    double m_wheelValue = -1.0;   // the value the cached disc was rendered at
};

class ColorsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ColorsPanel(QWidget *parent = nullptr);

    void setDocument(Document *doc);
    void activatePrimaryColorSlot();
    void activateSecondaryColorSlot();
    void toggleActiveColorSlot();
    void swapPrimaryAndSecondaryColors();
    // Redraws the reset/swap icons for the current colour scheme. They are
    // painted pixmaps, so a stylesheet change cannot recolour them.
    void refreshIcons();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onPrimaryClicked();
    void onSecondaryClicked();
    void onSwapClicked();
    void onWheelColorChanged(const QColor &color);
    void onSlidersChanged();
    void onHsvSlidersChanged();
    void onHexChanged();
    void onSwatchClicked(const QColor &color);
    void onSwatchRightClicked(const QColor &color);
    void updateFromDocument();
    void shrinkToFit();

private:
    void updateSliders(const QColor &color);
    void updateHsvSliders(const QColor &color);
    void updateHex(const QColor &color);
    // Takes a QBoxLayout, not a QLayout: the palette must be added with
    // addLayout(), which adopts the sub-layout. Plain addItem() does not, and the
    // swatch buttons then never get a parent widget and are never shown.
    void createSwatches(class QBoxLayout *layout);

    Document *m_document = nullptr;
    class QComboBox *m_slotCombo = nullptr;
    bool m_editingPrimary = true;

    QLabel *m_primarySwatch;
    QLabel *m_secondarySwatch;
    QPushButton *m_swapBtn;
    class QToolButton *m_resetBtn = nullptr;
    ColorWheelWidget *m_colorWheel;

    QSlider *m_redSlider, *m_greenSlider, *m_blueSlider, *m_alphaSlider;
    QSpinBox *m_redSpin, *m_greenSpin, *m_blueSpin, *m_alphaSpin;
    QSlider *m_hueSlider = nullptr, *m_satSlider = nullptr, *m_valSlider = nullptr;
    QSpinBox *m_hueSpin = nullptr, *m_satSpin = nullptr, *m_valSpin = nullptr;
    QLineEdit *m_hexEdit;

    QWidget *m_slidersWidget = nullptr;
    QWidget *m_hexWidget = nullptr;
    QWidget *m_hsvWidget = nullptr;

    bool m_updating = false;
};
