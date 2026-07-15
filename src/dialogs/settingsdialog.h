#pragma once

#include <QDialog>

class QListWidget;
class QStackedWidget;
class QComboBox;
class QCheckBox;
class QSlider;

// Mirrors paint.net's Settings dialog: a category list on the left, a stacked
// page on the right, and a Close button.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    // Emitted as soon as the user picks a new colour scheme / language, so the
    // application can restyle live.
    void colorSchemeChanged();
    void languageChanged();
    void canvasSettingsChanged();

private:
    QWidget *buildUserInterfacePage();
    QWidget *buildCanvasPage();
    QWidget *buildPlaceholderPage(const QString &text);

    QListWidget *m_categories;
    QStackedWidget *m_pages;

    QComboBox *m_schemeCombo;
    QComboBox *m_languageCombo;
    QSlider *m_checkerSlider;
};
