#include "settingsdialog.h"
#include "i18n.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(TR("Paramètres"));
    resize(560, 400);

    auto *root = new QVBoxLayout(this);
    auto *body = new QHBoxLayout;

    // ---- Category list (left) ----
    m_categories = new QListWidget;
    m_categories->setFixedWidth(160);
    m_categories->addItem(TR("Interface utilisateur"));
    m_categories->addItem(TR("Canevas"));
    m_categories->addItem(TR("Outils"));
    m_categories->addItem(TR("Aide"));
    body->addWidget(m_categories);

    // ---- Pages (right) ----
    m_pages = new QStackedWidget;
    m_pages->addWidget(buildUserInterfacePage());
    m_pages->addWidget(buildCanvasPage());
    m_pages->addWidget(buildPlaceholderPage(
        TR("Largeur du pinceau ( [ et ] pour ajuster )")));
    m_pages->addWidget(buildPlaceholderPage(
        TR("Astuce : F5–F8 affichent/masquent les fenêtres Outils, Historique, ")));
    body->addWidget(m_pages, 1);

    connect(m_categories, &QListWidget::currentRowChanged,
            m_pages, &QStackedWidget::setCurrentIndex);
    m_categories->setCurrentRow(0);

    root->addLayout(body, 1);

    // ---- Close button ----
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *closeBtn = new QPushButton(TR("Fermer"));
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);
}

QWidget *SettingsDialog::buildUserInterfacePage() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *form = new QFormLayout;

    // Color Scheme: Default / Light / Dark, applied immediately.
    m_schemeCombo = new QComboBox;
    m_schemeCombo->addItem(TR("Défaut (système)"), int(Theme::Scheme::Default));
    m_schemeCombo->addItem(TR("Clair"),            int(Theme::Scheme::Light));
    m_schemeCombo->addItem(TR("Sombre"),           int(Theme::Scheme::Dark));
    m_schemeCombo->setCurrentIndex(int(Theme::scheme()));
    connect(m_schemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        Theme::setScheme(static_cast<Theme::Scheme>(m_schemeCombo->itemData(i).toInt()));
        Theme::saveToSettings();
        emit colorSchemeChanged();
    });
    form->addRow(TR("Thème :"), m_schemeCombo);

    // Language: French / English, applied immediately.
    m_languageCombo = new QComboBox;
    m_languageCombo->addItem(TR("Français"), int(I18n::Lang::French));
    m_languageCombo->addItem(TR("Anglais"),  int(I18n::Lang::English));
    m_languageCombo->setCurrentIndex(int(I18n::language()));
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        I18n::setLanguage(static_cast<I18n::Lang>(m_languageCombo->itemData(i).toInt()));
        I18n::saveToSettings();
        emit languageChanged();
    });
    form->addRow(TR("Langue :"), m_languageCombo);

    layout->addLayout(form);

    auto *note = new QLabel(TR("Le changement de langue est appliqué immédiatement."));
    note->setStyleSheet("color: #4a90d9;");
    note->setWordWrap(true);
    layout->addWidget(note);

    layout->addStretch();
    return page;
}

QWidget *SettingsDialog::buildCanvasPage() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    QSettings s("PaintDali", "PaintDali");

    auto *label = new QLabel(TR("Luminosité du damier de transparence :"));
    layout->addWidget(label);

    m_checkerSlider = new QSlider(Qt::Horizontal);
    m_checkerSlider->setRange(0, 100);
    m_checkerSlider->setValue(s.value("canvas/checkerBrightness", 80).toInt());
    connect(m_checkerSlider, &QSlider::valueChanged, this, [this](int v) {
        QSettings s2("PaintDali", "PaintDali");
        s2.setValue("canvas/checkerBrightness", v);
        emit canvasSettingsChanged();
    });
    layout->addWidget(m_checkerSlider);

    layout->addStretch();
    return page;
}

QWidget *SettingsDialog::buildPlaceholderPage(const QString &text) {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *label = new QLabel(text);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignTop);
    layout->addWidget(label);
    layout->addStretch();
    return page;
}
