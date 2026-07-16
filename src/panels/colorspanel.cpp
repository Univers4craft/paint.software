#include "colorspanel.h"
#include "i18n.h"
#include <QTimer>
#include <QDockWidget>
#include "core/document.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolButton>
#include "../theme.h"
#include <QComboBox>
#include <QEvent>
#include <cmath>

// ---- ColorWheelWidget ----

ColorWheelWidget::ColorWheelWidget(QWidget *parent) : QWidget(parent) {
    // Floor only — the preferred height comes from sizeHint(), which sizes the
    // disc to match the palette row below it. Keeping the floor low lets the
    // user shrink the panel by hand without the layout fighting back.
    setMinimumSize(140, 140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ColorWheelWidget::setColor(const QColor &color) {
    m_color = color;
    m_hue = color.hsvHueF();
    if (m_hue < 0) m_hue = 0;
    m_sat = color.hsvSaturationF();
    m_val = color.valueF();
    update();
}

// paint.net uses a single filled HSV disc: hue runs around the circle,
// saturation from the (white) centre out to the fully-saturated rim. Value is
// controlled separately by the V slider.
void ColorWheelWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = std::min(width(), height());
    int cx = width() / 2, cy = height() / 2;
    m_wheelRadius = side / 2 - 2;
    m_innerRadius = 0;

    if (m_wheelImage.isNull() || m_wheelImage.size() != size())
        rebuildWheel();

    painter.drawImage(0, 0, m_wheelImage);

    // Rim
    painter.setPen(QPen(QColor(120, 120, 120), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPoint(cx, cy), m_wheelRadius, m_wheelRadius);

    // Current-colour indicator: angle = hue, distance from centre = saturation.
    // The dot is FILLED with the actual selected colour, so an achromatic colour
    // (black / white / grey) that sits at the centre is still recognisable — the
    // centre of the disc itself is white, which otherwise made a black selection
    // look wrong.
    const double angle = m_hue * 2 * M_PI;
    const double r = m_sat * m_wheelRadius;
    const int hx = cx + static_cast<int>(r * std::cos(angle));
    const int hy = cy - static_cast<int>(r * std::sin(angle));
    painter.setBrush(m_color);
    painter.setPen(QPen(Qt::white, 2));
    painter.drawEllipse(QPoint(hx, hy), 6, 6);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(QPoint(hx, hy), 7, 7);
}

void ColorWheelWidget::pickAt(const QPoint &pos) {
    const int cx = width() / 2, cy = height() / 2;
    const double dx = pos.x() - cx, dy = pos.y() - cy;
    const double dist = std::sqrt(dx * dx + dy * dy);
    if (m_wheelRadius <= 0) return;

    double angle = std::atan2(-dy, dx);
    if (angle < 0) angle += 2 * M_PI;
    m_hue = angle / (2 * M_PI);
    m_sat = qBound(0.0, dist / m_wheelRadius, 1.0);   // clamp outside the rim

    // The disc only carries hue and saturation. If the current colour is fully
    // dark (black is the default), picking a hue would still yield black — so
    // lift the value to full, which is what the user visibly clicked on.
    if (m_val <= 0.01) m_val = 1.0;

    // Preserve the current alpha instead of silently resetting it to opaque.
    const double alpha = m_color.isValid() ? m_color.alphaF() : 1.0;
    m_color = QColor::fromHsvF(m_hue, m_sat, m_val, alpha);
    emit colorChanged(m_color);
    update();
}

void ColorWheelWidget::mousePressEvent(QMouseEvent *event) {
    const int cx = width() / 2, cy = height() / 2;
    const double dx = event->pos().x() - cx, dy = event->pos().y() - cy;
    if (std::sqrt(dx * dx + dy * dy) <= m_wheelRadius + 2) {
        m_draggingWheel = true;
        pickAt(event->pos());
    }
}

void ColorWheelWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_draggingWheel) pickAt(event->pos());
}

void ColorWheelWidget::mouseReleaseEvent(QMouseEvent *) {
    m_draggingWheel = false;
    m_draggingSquare = false;
}

void ColorWheelWidget::resizeEvent(QResizeEvent *) {
    m_wheelImage = QImage();
}

void ColorWheelWidget::rebuildWheel() {
    const int w = width(), h = height();
    if (w <= 0 || h <= 0) return;
    m_wheelImage = QImage(w, h, QImage::Format_ARGB32);
    m_wheelImage.fill(Qt::transparent);
    m_wheelValue = m_val;

    const int cx = w / 2, cy = h / 2;
    const double R = m_wheelRadius;
    if (R <= 0) return;

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(m_wheelImage.scanLine(y));
        for (int x = 0; x < w; ++x) {
            const double dx = x - cx, dy = y - cy;
            const double dist = std::sqrt(dx * dx + dy * dy);
            if (dist > R) continue;

            double angle = std::atan2(-dy, dx);
            if (angle < 0) angle += 2 * M_PI;
            const double hue = angle / (2 * M_PI);
            const double sat = dist / R;
            // The disc always shows full-value hues (it is a hue/saturation
            // picker); brightness lives on the separate V slider. Rendering it
            // at m_val would turn the whole wheel black when black is selected.
            QColor c = QColor::fromHsvF(hue, sat, 1.0);

            // Feather the rim so the disc doesn't look jagged.
            const double edge = R - dist;
            const int a = (edge < 1.0) ? static_cast<int>(255 * qBound(0.0, edge, 1.0)) : 255;
            line[x] = qRgba(c.red(), c.green(), c.blue(), a);
        }
    }
}

// ---- ColorsPanel ----

ColorsPanel::ColorsPanel(QWidget *parent) : QWidget(parent) {
    // Without this a plain QWidget subclass ignores the stylesheet's
    // background-color, so the panel stayed dark under the light scheme.
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);

    // Combo Primaire/Secondaire + bouton Plus
    auto *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    m_slotCombo = new QComboBox;
    auto *primaryCombo = m_slotCombo;
    primaryCombo->addItem(TR("Primaire"));
    primaryCombo->addItem(TR("Secondaire"));
    primaryCombo->setFixedHeight(20);
    headerLayout->addWidget(primaryCombo);
    headerLayout->addStretch();
    auto *moreBtn = new QPushButton(TR("Plus >>"));
    moreBtn->setFixedHeight(20);
    headerLayout->addWidget(moreBtn);
    layout->addLayout(headerLayout);

    connect(primaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx == 0) onPrimaryClicked();
        else onSecondaryClicked();
    });

    // Primary/Secondary overlapping swatches (Paint.NET style)
    auto *swatchContainer = new QWidget;
    swatchContainer->setFixedSize(70, 50);
    // Primary swatch: larger, in front
    m_primarySwatch = new QLabel(swatchContainer);
    m_primarySwatch->setGeometry(0, 0, 32, 32);
    m_primarySwatch->setStyleSheet("background-color: black; border: 2px solid white;");
    m_primarySwatch->setCursor(Qt::PointingHandCursor);
    m_primarySwatch->installEventFilter(this);
    // Secondary swatch: smaller, behind and offset
    m_secondarySwatch = new QLabel(swatchContainer);
    m_secondarySwatch->setGeometry(16, 16, 32, 32);
    m_secondarySwatch->setStyleSheet("background-color: white; border: 2px solid #808080;");
    m_secondarySwatch->setCursor(Qt::PointingHandCursor);
    m_secondarySwatch->installEventFilter(this);
    // Raise primary on top
    m_primarySwatch->raise();

    m_swapBtn = new QPushButton;   // icon drawn by refreshIcons(), scheme-aware
    m_swapBtn->setFixedSize(20, 20);
    m_swapBtn->setToolTip(TR("Permuter les couleurs"));
    m_swapBtn->setParent(swatchContainer);
    m_swapBtn->move(50, 0);

    auto *swatchRow = new QHBoxLayout;
    swatchRow->addWidget(swatchContainer);
    swatchRow->addStretch();
    layout->addLayout(swatchRow);

    // Color wheel
    m_colorWheel = new ColorWheelWidget;
    m_colorWheel->setMinimumHeight(150);
    layout->addWidget(m_colorWheel, 1);

    // Hex input (created early to avoid null pointer in onSlidersChanged)
    m_hexEdit = new QLineEdit("#000000");
    m_hexEdit->setMaxLength(9);
    m_hexEdit->setFixedHeight(20);

    // Small utility buttons row below wheel (like Paint.NET)
    auto *utilRow = new QHBoxLayout;
    utilRow->setSpacing(2);
    m_resetBtn = new QToolButton;   // icon drawn by refreshIcons(), scheme-aware
    m_resetBtn->setFixedSize(20, 20);
    m_resetBtn->setToolTip(TR("Réinitialiser les couleurs par défaut"));
    connect(m_resetBtn, &QToolButton::clicked, this, [this]() {
        if (m_document) { m_document->setPrimaryColor(Qt::black); m_document->setSecondaryColor(Qt::white); }
    });
    refreshIcons();
    utilRow->addWidget(m_resetBtn);
    utilRow->addWidget(m_swapBtn);
    utilRow->addStretch();
    layout->addLayout(utilRow);

    // RGB sliders — visible by default, labeled RVB (French)
    m_slidersWidget = new QWidget;
    auto *sliderGrid = new QGridLayout(m_slidersWidget);
    sliderGrid->setContentsMargins(0, 0, 0, 0);
    sliderGrid->setSpacing(1);
    // RVB header
    auto *rvbHeader = new QLabel("RVB");
    rvbHeader->setStyleSheet("font-weight: bold; font-size: 10px; color: #555;");
    sliderGrid->addWidget(rvbHeader, 0, 0, 1, 3);
    auto makeSlider = [&](const QString &label, int row, QSlider *&slider, QSpinBox *&spin) {
        auto *lbl = new QLabel(label);
        lbl->setFixedWidth(14);
        sliderGrid->addWidget(lbl, row, 0);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 255);
        sliderGrid->addWidget(slider, row, 1);
        spin = new QSpinBox;
        spin->setRange(0, 255);
        spin->setFixedWidth(45);
        spin->setFixedHeight(18);
        sliderGrid->addWidget(spin, row, 2);
        connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
        connect(slider, &QSlider::valueChanged, this, &ColorsPanel::onSlidersChanged);
    };

    makeSlider("R:", 1, m_redSlider, m_redSpin);
    makeSlider("V:", 2, m_greenSlider, m_greenSpin);
    makeSlider("B:", 3, m_blueSlider, m_blueSpin);
    makeSlider("A:", 4, m_alphaSlider, m_alphaSpin);
    m_alphaSlider->setValue(255);
    // Collapsed by default (paint.net opens with the "More >>" panel closed).
    m_slidersWidget->setVisible(false);
    layout->addWidget(m_slidersWidget);

    // HSV sliders (also hidden by default)
    m_hsvWidget = new QWidget;
    auto *hsvGrid = new QGridLayout(m_hsvWidget);
    hsvGrid->setContentsMargins(0, 0, 0, 0);
    hsvGrid->setSpacing(1);

    // HSV header label
    auto *hsvHeader = new QLabel("TSV");
    hsvHeader->setStyleSheet("font-weight: bold; font-size: 10px; color: #555;");
    hsvGrid->addWidget(hsvHeader, 0, 0, 1, 3);

    auto makeHsvSlider = [&](const QString &label, int row, int maxVal, QSlider *&slider, QSpinBox *&spin) {
        auto *lbl = new QLabel(label);
        lbl->setFixedWidth(14);
        hsvGrid->addWidget(lbl, row, 0);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, maxVal);
        hsvGrid->addWidget(slider, row, 1);
        spin = new QSpinBox;
        spin->setRange(0, maxVal);
        spin->setFixedWidth(45);
        spin->setFixedHeight(18);
        hsvGrid->addWidget(spin, row, 2);
        connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
        connect(slider, &QSlider::valueChanged, this, &ColorsPanel::onHsvSlidersChanged);
    };

    makeHsvSlider("T:", 1, 360, m_hueSlider, m_hueSpin);
    makeHsvSlider("S:", 2, 100, m_satSlider, m_satSpin);
    makeHsvSlider("V:", 3, 100, m_valSlider, m_valSpin);
    m_hsvWidget->setVisible(false);
    layout->addWidget(m_hsvWidget);

    // Hex input layout (visible by default)
    m_hexWidget = new QWidget;
    auto *hexLayout = new QHBoxLayout(m_hexWidget);
    hexLayout->setContentsMargins(0, 0, 0, 0);
    hexLayout->addWidget(new QLabel(TR("Hexa :")));
    hexLayout->addWidget(m_hexEdit);
    m_hexWidget->setVisible(false);
    layout->addWidget(m_hexWidget);

    // Color swatches palette at bottom
    createSwatches(layout);

    connect(m_swapBtn, &QPushButton::clicked, this, &ColorsPanel::onSwapClicked);
    connect(m_colorWheel, &ColorWheelWidget::colorChanged, this, &ColorsPanel::onWheelColorChanged);
    connect(m_hexEdit, &QLineEdit::editingFinished, this, &ColorsPanel::onHexChanged);

    // "<< Moins" / "Plus >>" collapses the advanced controls (RGB/HSV/hex).
    connect(moreBtn, &QPushButton::clicked, this, [this, moreBtn]() {
        bool show = !m_slidersWidget->isVisible();
        m_slidersWidget->setVisible(show);
        m_hsvWidget->setVisible(show);
        m_hexWidget->setVisible(show);
        moreBtn->setText(show ? TR("<< Moins") : TR("Plus >>"));
        shrinkToFit();
    });
}

void ColorsPanel::shrinkToFit() {
    // Hiding the sliders only frees layout space; the floating utility window
    // keeps its old height unless we resize it too. Walk up to the QDockWidget
    // and shrink/grow it to the panel's natural height.
    layout()->activate();
    adjustSize();

    QWidget *w = parentWidget();
    while (w && !qobject_cast<QDockWidget*>(w))
        w = w->parentWidget();
    auto *dock = qobject_cast<QDockWidget*>(w);
    if (!dock || !dock->isFloating()) return;

    // Deferred: let the layout settle before asking for the new size hint.
    QTimer::singleShot(0, dock, [dock]() {
        dock->resize(dock->width(), dock->sizeHint().height());
    });
}

bool ColorsPanel::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == m_primarySwatch) {
            onPrimaryClicked();
            return true;
        } else if (obj == m_secondarySwatch) {
            onSecondaryClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ColorsPanel::setDocument(Document *doc) {
    m_document = doc;
    if (doc) {
        connect(doc, &Document::primaryColorChanged, this, &ColorsPanel::updateFromDocument);
        connect(doc, &Document::secondaryColorChanged, this, &ColorsPanel::updateFromDocument);
        updateFromDocument();
    }
}

void ColorsPanel::activatePrimaryColorSlot() {
    onPrimaryClicked();
}

void ColorsPanel::activateSecondaryColorSlot() {
    onSecondaryClicked();
}

void ColorsPanel::toggleActiveColorSlot() {
    if (m_editingPrimary) activateSecondaryColorSlot();
    else activatePrimaryColorSlot();
}

void ColorsPanel::swapPrimaryAndSecondaryColors() {
    onSwapClicked();
}

// Both slot selectors funnel through updateFromDocument(), which is the single
// place that refreshes the swatch borders, the combo, the wheel, the RGB/HSV
// sliders and the hex field. Previously onSecondaryClicked() skipped the border
// and combo refresh, so the panel kept showing the primary slot as active while
// you were actually editing the secondary one.
void ColorsPanel::onPrimaryClicked() {
    m_editingPrimary = true;
    updateFromDocument();
}

void ColorsPanel::onSecondaryClicked() {
    m_editingPrimary = false;
    updateFromDocument();
}

void ColorsPanel::onSwapClicked() {
    if (m_document) {
        m_document->swapColors();
        updateFromDocument();
    }
}

void ColorsPanel::onWheelColorChanged(const QColor &color) {
    if (m_updating) return;
    m_updating = true;
    updateSliders(color);
    updateHsvSliders(color);
    updateHex(color);
    if (m_document) {
        if (m_editingPrimary) m_document->setPrimaryColor(color);
        else m_document->setSecondaryColor(color);
    }
    m_updating = false;
}

void ColorsPanel::onSlidersChanged() {
    if (m_updating) return;
    m_updating = true;
    QColor color(m_redSlider->value(), m_greenSlider->value(), m_blueSlider->value(), m_alphaSlider->value());
    m_colorWheel->setColor(color);
    updateHex(color);
    updateHsvSliders(color);
    if (m_document) {
        if (m_editingPrimary) m_document->setPrimaryColor(color);
        else m_document->setSecondaryColor(color);
    }
    m_updating = false;
}

void ColorsPanel::onHsvSlidersChanged() {
    if (m_updating) return;
    m_updating = true;
    QColor color = QColor::fromHsv(m_hueSlider->value(), m_satSlider->value() * 255 / 100, m_valSlider->value() * 255 / 100, m_alphaSlider->value());
    m_colorWheel->setColor(color);
    updateSliders(color);
    updateHex(color);
    if (m_document) {
        if (m_editingPrimary) m_document->setPrimaryColor(color);
        else m_document->setSecondaryColor(color);
    }
    m_updating = false;
}

void ColorsPanel::onHexChanged() {
    if (m_updating) return;
    m_updating = true;
    QColor color(m_hexEdit->text());
    if (color.isValid()) {
        m_colorWheel->setColor(color);
        updateSliders(color);
        updateHsvSliders(color);
        if (m_document) {
            if (m_editingPrimary) m_document->setPrimaryColor(color);
            else m_document->setSecondaryColor(color);
        }
    }
    m_updating = false;
}

void ColorsPanel::updateFromDocument() {
    if (!m_document) return;
    m_updating = true;
    QColor primary = m_document->primaryColor();
    QColor secondary = m_document->secondaryColor();

    // The active slot gets a bright outline; the inactive one a muted one.
    m_primarySwatch->setStyleSheet(
        QString("background-color: %1; border: %2;")
            .arg(primary.name(), m_editingPrimary ? "3px solid #4a90d9" : "2px solid gray"));
    m_secondarySwatch->setStyleSheet(
        QString("background-color: %1; border: %2;")
            .arg(secondary.name(), m_editingPrimary ? "2px solid gray" : "3px solid #4a90d9"));
    // Whichever slot is active is drawn in front.
    if (m_editingPrimary) m_primarySwatch->raise();
    else                  m_secondarySwatch->raise();

    if (m_slotCombo) {
        m_slotCombo->blockSignals(true);
        m_slotCombo->setCurrentIndex(m_editingPrimary ? 0 : 1);
        m_slotCombo->blockSignals(false);
    }

    QColor current = m_editingPrimary ? primary : secondary;
    m_colorWheel->setColor(current);
    updateSliders(current);
    updateHsvSliders(current);
    updateHex(current);
    m_updating = false;
}

void ColorsPanel::updateSliders(const QColor &color) {
    m_redSlider->blockSignals(true);
    m_greenSlider->blockSignals(true);
    m_blueSlider->blockSignals(true);
    m_alphaSlider->blockSignals(true);
    m_redSlider->setValue(color.red());
    m_greenSlider->setValue(color.green());
    m_blueSlider->setValue(color.blue());
    m_alphaSlider->setValue(color.alpha());
    m_redSlider->blockSignals(false);
    m_greenSlider->blockSignals(false);
    m_blueSlider->blockSignals(false);
    m_alphaSlider->blockSignals(false);
    m_redSpin->setValue(color.red());
    m_greenSpin->setValue(color.green());
    m_blueSpin->setValue(color.blue());
    m_alphaSpin->setValue(color.alpha());
}

void ColorsPanel::updateHex(const QColor &color) {
    m_hexEdit->setText(color.name(QColor::HexArgb));
}

void ColorsPanel::updateHsvSliders(const QColor &color) {
    if (!m_hueSlider || !m_satSlider || !m_valSlider) return;
    m_hueSlider->blockSignals(true);
    m_satSlider->blockSignals(true);
    m_valSlider->blockSignals(true);
    int h = color.hsvHue(); if (h < 0) h = 0;
    m_hueSlider->setValue(h);
    m_satSlider->setValue(color.hsvSaturation() * 100 / 255);
    m_valSlider->setValue(color.value() * 100 / 255);
    m_hueSlider->blockSignals(false);
    m_satSlider->blockSignals(false);
    m_valSlider->blockSignals(false);
    m_hueSpin->setValue(h);
    m_satSpin->setValue(color.hsvSaturation() * 100 / 255);
    m_valSpin->setValue(color.value() * 100 / 255);
}

void ColorsPanel::refreshIcons() {
    // Both icons used to be drawn in a fixed dark grey — the same colour as the
    // dark scheme's panel, so they were invisible. Follow the active scheme.
    const QColor fg = Theme::isDark() ? QColor(225, 225, 225) : QColor(60, 60, 60);

    if (m_resetBtn) {
        QPixmap pm(14, 14);
        pm.fill(Qt::transparent);
        QPainter rp(&pm);
        rp.setRenderHint(QPainter::Antialiasing, true);
        rp.setPen(QPen(fg, 1.5));
        rp.setBrush(Qt::NoBrush);
        rp.drawArc(QRect(2, 2, 10, 10), 45 * 16, 270 * 16);
        rp.setBrush(fg);
        rp.setPen(Qt::NoPen);
        QPointF a[] = {QPointF(7, 1), QPointF(4, 3.5), QPointF(7, 6)};
        rp.drawPolygon(a, 3);
        rp.end();
        m_resetBtn->setIcon(QIcon(pm));
        m_resetBtn->setIconSize(QSize(14, 14));
    }

    if (m_swapBtn) {
        QPixmap pm(14, 14);
        pm.fill(Qt::transparent);
        QPainter sp(&pm);
        sp.setRenderHint(QPainter::Antialiasing, true);
        sp.setPen(QPen(fg, 1.3));
        sp.drawLine(3, 4, 11, 4);
        sp.drawLine(9, 2, 11, 4); sp.drawLine(9, 6, 11, 4);
        sp.drawLine(3, 10, 11, 10);
        sp.drawLine(3, 8, 5, 10); sp.drawLine(3, 12, 5, 10);
        sp.end();
        m_swapBtn->setIcon(QIcon(pm));
        m_swapBtn->setIconSize(QSize(14, 14));
    }
}

void ColorsPanel::createSwatches(QBoxLayout *parentLayout) {
    // paint.net's default palette: 32 colours laid out as 2 rows of 16. The top
    // row is the fully-saturated hues, the bottom row their darker halves.
    static const QColor swatchColors[] = {
        // Row 1 — black, mid grey, then the bright hues
        QColor(0x00,0x00,0x00), QColor(0x40,0x40,0x40), QColor(0xFF,0x00,0x00), QColor(0xFF,0x6A,0x00),
        QColor(0xFF,0xD8,0x00), QColor(0xB6,0xFF,0x00), QColor(0x4C,0xFF,0x00), QColor(0x00,0xFF,0x21),
        QColor(0x00,0xFF,0x90), QColor(0x00,0xFF,0xFF), QColor(0x00,0x94,0xFF), QColor(0x00,0x26,0xFF),
        QColor(0x48,0x00,0xFF), QColor(0xB2,0x00,0xFF), QColor(0xFF,0x00,0xDC), QColor(0xFF,0x00,0x6E),
        // Row 2 — white, light grey, then the same hues at half value
        QColor(0xFF,0xFF,0xFF), QColor(0x80,0x80,0x80), QColor(0x7F,0x00,0x00), QColor(0x7F,0x33,0x00),
        QColor(0x7F,0x6A,0x00), QColor(0x5B,0x7F,0x00), QColor(0x26,0x7F,0x00), QColor(0x00,0x7F,0x0E),
        QColor(0x00,0x7F,0x46), QColor(0x00,0x7F,0x7F), QColor(0x00,0x4A,0x7F), QColor(0x00,0x13,0x7F),
        QColor(0x21,0x00,0x7F), QColor(0x57,0x00,0x7F), QColor(0x7F,0x00,0x6E), QColor(0x7F,0x00,0x37),
    };

    // Swatch size, border included. 16 of these across sets the panel's width,
    // so keep it small: the wheel above is sized to match this row's width.
    const int kSwatch = 11;

    auto *grid = new QGridLayout;
    grid->setSpacing(1);
    grid->setContentsMargins(0, 4, 0, 0);

    int cols = 16;   // 2 rows of 16, like paint.net
    int count = sizeof(swatchColors) / sizeof(swatchColors[0]);
    for (int i = 0; i < count; ++i) {
        auto *btn = new QToolButton;
        btn->setFixedSize(kSwatch, kSwatch);
        btn->setAutoRaise(true);
        QString hex = swatchColors[i].name();
        // The stylesheet sizes the *content* box, so the 1px border and any
        // padding are added on top: state both, or each cell silently ends up
        // half again as wide as asked and the whole panel bloats.
        btn->setStyleSheet(QString("QToolButton { background-color: %1; border: 1px solid #888;"
                                   " padding: 0px; margin: 0px;"
                                   " min-width: %2px; min-height: %2px;"
                                   " max-width: %2px; max-height: %2px; }")
                               .arg(hex).arg(kSwatch - 2));
        btn->setToolTip(QString("<b>%1</b><br>%2")
            .arg(hex, TR("Clic : couleur primaire · Clic droit : couleur secondaire")));
        QColor c = swatchColors[i];
        connect(btn, &QToolButton::clicked, this, [this, c]() { onSwatchClicked(c); });

        // Right-click assigns the swatch to the *other* slot, like paint.net.
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QToolButton::customContextMenuRequested, this, [this, c]() {
            onSwatchRightClicked(c);
        });
        grid->addWidget(btn, i / cols, i % cols);
    }
    // addLayout, not addItem: only addLayout adopts the sub-layout and reparents
    // the swatch buttons into this panel. With addItem they stay parentless and
    // the whole palette is silently invisible.
    parentLayout->addLayout(grid);
}

void ColorsPanel::onSwatchRightClicked(const QColor &color) {
    // Assign to the slot that is NOT currently being edited.
    if (!m_document) return;
    if (m_editingPrimary) m_document->setSecondaryColor(color);
    else                  m_document->setPrimaryColor(color);
    updateFromDocument();
}

void ColorsPanel::onSwatchClicked(const QColor &color) {
    if (m_updating) return;
    m_updating = true;
    m_colorWheel->setColor(color);
    updateSliders(color);
    updateHsvSliders(color);
    updateHex(color);
    if (m_document) {
        if (m_editingPrimary) m_document->setPrimaryColor(color);
        else m_document->setSecondaryColor(color);
    }
    m_updating = false;
}
