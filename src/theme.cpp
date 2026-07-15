#include "theme.h"

#include <QSettings>
#include <QApplication>
#include <QPalette>
#include <QStyleHints>

namespace {

Theme::Scheme g_scheme = Theme::Scheme::Default;

bool osPrefersDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (auto *hints = QGuiApplication::styleHints())
        return hints->colorScheme() == Qt::ColorScheme::Dark;
#endif
    // Fallback: inspect the palette the platform theme gave us.
    const QPalette pal = QApplication::palette();
    return pal.color(QPalette::Window).lightness() < 128;
}

// ---------------------------------------------------------------------------
// Light scheme — paint.net's default Windows light look: neutral grays, blue
// accent used only for hover/selection.
// ---------------------------------------------------------------------------
const char *kLight = R"(
    QMainWindow, QWidget { color: #1f1f1f; font-size: 11px; }
    QMainWindow { background-color: #f0f0f0; }
    QDialog { background-color: #f0f0f0; }

    QMenuBar { background-color: #f0f0f0; color: #1f1f1f; border-bottom: 1px solid #dcdcdc; font-size: 12px; min-height: 24px; }
    QMenuBar::item { padding: 4px 9px; background: transparent; }
    QMenuBar::item:selected { background-color: #cde4fa; }
    QMenuBar::item:pressed { background-color: #b6d8f5; }
    QMenu { background-color: #ffffff; color: #1f1f1f; border: 1px solid #c8c8c8; }
    QMenu::item { padding: 5px 26px 5px 26px; }
    QMenu::item:selected { background-color: #cde4fa; }
    QMenu::item:disabled { color: #a0a0a0; }
    QMenu::separator { height: 1px; background: #e0e0e0; margin: 3px 6px; }

    QToolBar#FixedToolbar { background-color: #f0f0f0; border-bottom: 1px solid #dcdcdc; spacing: 1px; padding: 1px 2px; min-height: 26px; }
    QToolBar#VariableToolbar { background-color: #f7f7f7; border-bottom: 1px solid #dcdcdc; spacing: 2px; padding: 1px 2px; min-height: 28px; }
    QToolBar::separator { background: #d4d4d4; width: 1px; margin: 3px 3px; }

    QToolButton { color: #1f1f1f; background-color: transparent; border: 1px solid transparent; border-radius: 3px; padding: 2px; }
    QToolButton:hover { background-color: #e3eff9; border-color: #a8cdea; }
    QToolButton:checked { background-color: #cde4fa; border-color: #7ab0dd; }
    QToolButton:pressed { background-color: #b6d8f5; }

    QDockWidget { font-size: 11px; color: #1f1f1f; background-color: #f0f0f0; }
    QDockWidget::title { background-color: #e4e4e4; border: 1px solid #cfcfcf; padding: 4px 6px; color: #333333; font-weight: bold; text-align: left; }
    #ToolsPalette, LayersPanel, ColorsPanel, HistoryPanel, ToolOptionsPanel { background-color: #f0f0f0; }
    QDockWidget::close-button, QDockWidget::float-button {
        subcontrol-position: top right; subcontrol-origin: margin;
        background: transparent; border: 1px solid transparent;
        border-radius: 2px; width: 14px; height: 14px; top: 2px;
    }
    QDockWidget::close-button { right: 3px; }
    QDockWidget::float-button { right: 19px; }
    QDockWidget::close-button:hover { background: #e8a0a0; border-color: #d08080; }
    QDockWidget::float-button:hover { background: #cde4fa; border-color: #7ab0dd; }

    QLabel { color: #1f1f1f; }
    QGroupBox { border: 1px solid #d0d0d0; border-radius: 3px; margin-top: 8px; padding-top: 6px; }
    QGroupBox::title { subcontrol-origin: margin; left: 7px; padding: 0 3px; }

    QSlider::groove:horizontal { height: 4px; background: #d0d0d0; border-radius: 2px; }
    QSlider::sub-page:horizontal { background: #7ab0dd; border-radius: 2px; }
    QSlider::handle:horizontal { background: #fdfdfd; border: 1px solid #8a8a8a; width: 9px; margin: -5px 0; border-radius: 2px; }
    QSlider::handle:horizontal:hover { border-color: #4a90d9; }

    QListWidget { background-color: #ffffff; color: #1f1f1f; border: 1px solid #c8c8c8; }
    QListWidget::item { padding: 2px; }
    QListWidget::item:selected { background-color: #cde4fa; color: #1f1f1f; }

    QPushButton { background-color: #f0f0f0; color: #1f1f1f; border: 1px solid #b4b4b4; padding: 3px 7px; border-radius: 3px; font-size: 11px; }
    QPushButton:hover { background-color: #e3eff9; border-color: #a8cdea; }
    QPushButton:pressed { background-color: #cde4fa; }
    QPushButton:checked { background-color: #cde4fa; border-color: #7ab0dd; }

    QSpinBox, QComboBox, QLineEdit { background-color: #ffffff; color: #1f1f1f; border: 1px solid #b4b4b4; padding: 1px 3px; border-radius: 2px; font-size: 11px; }
    QSpinBox:focus, QComboBox:focus, QLineEdit:focus { border-color: #4a90d9; }
    QComboBox::drop-down { border: none; width: 16px; }
    QComboBox QAbstractItemView { background-color: #ffffff; color: #1f1f1f; selection-background-color: #cde4fa; selection-color: #1f1f1f; }
    QCheckBox { color: #1f1f1f; font-size: 11px; spacing: 4px; }

    QTabWidget::pane { border: 1px solid #c8c8c8; background: #f7f7f7; }
    QTabBar::tab { background: #e6e6e6; color: #1f1f1f; padding: 5px 12px; border: 1px solid #c8c8c8; border-bottom: none; }
    QTabBar::tab:selected { background: #f7f7f7; }

    QStatusBar { background-color: #f0f0f0; color: #333; border-top: 1px solid #dcdcdc; font-size: 11px; }
    QStatusBar QLabel { color: #333; padding: 0 3px; }
    QStatusBar::item { border: none; }

    QScrollBar:vertical { background: #f4f4f4; width: 14px; margin: 0; }
    QScrollBar::handle:vertical { background: #c6c6c6; border-radius: 3px; min-height: 24px; }
    QScrollBar::handle:vertical:hover { background: #a8a8a8; }
    QScrollBar:horizontal { background: #f4f4f4; height: 14px; margin: 0; }
    QScrollBar::handle:horizontal { background: #c6c6c6; border-radius: 3px; min-width: 24px; }
    QScrollBar::handle:horizontal:hover { background: #a8a8a8; }
    QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)";

// ---------------------------------------------------------------------------
// Dark scheme — paint.net's dark color scheme: near-black chrome, #2b2b2b
// surfaces, light gray text, same blue accent.
// ---------------------------------------------------------------------------
const char *kDark = R"(
    QMainWindow, QWidget { color: #f0f0f0; font-size: 11px; }
    QMainWindow { background-color: #2b2b2b; }
    QDialog { background-color: #2b2b2b; }

    QMenuBar { background-color: #2b2b2b; color: #f0f0f0; border-bottom: 1px solid #1e1e1e; font-size: 12px; min-height: 24px; }
    QMenuBar::item { padding: 4px 9px; background: transparent; }
    QMenuBar::item:selected { background-color: #3f5f7f; }
    QMenuBar::item:pressed { background-color: #2d6ba3; }
    QMenu { background-color: #333333; color: #f0f0f0; border: 1px solid #4a4a4a; }
    QMenu::item { padding: 5px 26px 5px 26px; }
    QMenu::item:selected { background-color: #2d6ba3; }
    QMenu::item:disabled { color: #7a7a7a; }
    QMenu::separator { height: 1px; background: #4a4a4a; margin: 3px 6px; }

    QToolBar#FixedToolbar { background-color: #2b2b2b; border-bottom: 1px solid #1e1e1e; spacing: 1px; padding: 1px 2px; min-height: 26px; }
    QToolBar#VariableToolbar { background-color: #323232; border-bottom: 1px solid #1e1e1e; spacing: 2px; padding: 1px 2px; min-height: 28px; }
    QToolBar::separator { background: #4a4a4a; width: 1px; margin: 3px 3px; }

    QToolButton { color: #f0f0f0; background-color: transparent; border: 1px solid transparent; border-radius: 3px; padding: 2px; }
    QToolButton:hover { background-color: #3d3d3d; border-color: #5a5a5a; }
    QToolButton:checked { background-color: #2d6ba3; border-color: #4a90d9; }
    QToolButton:pressed { background-color: #2d6ba3; }

    QDockWidget { font-size: 11px; color: #f0f0f0; background-color: #2b2b2b; }
    QDockWidget::title { background-color: #3a3a3a; border: 1px solid #4a4a4a; padding: 4px 6px; color: #e8e8e8; font-weight: bold; text-align: left; }
    #ToolsPalette, LayersPanel, ColorsPanel, HistoryPanel, ToolOptionsPanel { background-color: #2b2b2b; }
    QDockWidget::close-button, QDockWidget::float-button {
        subcontrol-position: top right; subcontrol-origin: margin;
        background: transparent; border: 1px solid transparent;
        border-radius: 2px; width: 14px; height: 14px; top: 2px;
    }
    QDockWidget::close-button { right: 3px; }
    QDockWidget::float-button { right: 19px; }
    QDockWidget::close-button:hover { background: #a04040; border-color: #c05050; }
    QDockWidget::float-button:hover { background: #2d6ba3; border-color: #4a90d9; }

    QLabel { color: #f0f0f0; }
    QGroupBox { border: 1px solid #4a4a4a; border-radius: 3px; margin-top: 8px; padding-top: 6px; }
    QGroupBox::title { subcontrol-origin: margin; left: 7px; padding: 0 3px; }

    QSlider::groove:horizontal { height: 4px; background: #4a4a4a; border-radius: 2px; }
    QSlider::sub-page:horizontal { background: #4a90d9; border-radius: 2px; }
    QSlider::handle:horizontal { background: #d0d0d0; border: 1px solid #6a6a6a; width: 9px; margin: -5px 0; border-radius: 2px; }
    QSlider::handle:horizontal:hover { border-color: #4a90d9; }

    QListWidget { background-color: #1e1e1e; color: #f0f0f0; border: 1px solid #4a4a4a; }
    QListWidget::item { padding: 2px; }
    QListWidget::item:selected { background-color: #2d6ba3; color: #ffffff; }

    QPushButton { background-color: #3a3a3a; color: #f0f0f0; border: 1px solid #5a5a5a; padding: 3px 7px; border-radius: 3px; font-size: 11px; }
    QPushButton:hover { background-color: #464646; border-color: #6a6a6a; }
    QPushButton:pressed { background-color: #2d6ba3; }
    QPushButton:checked { background-color: #2d6ba3; border-color: #4a90d9; }

    QSpinBox, QComboBox, QLineEdit { background-color: #1e1e1e; color: #f0f0f0; border: 1px solid #5a5a5a; padding: 1px 3px; border-radius: 2px; font-size: 11px; }
    QSpinBox:focus, QComboBox:focus, QLineEdit:focus { border-color: #4a90d9; }
    QComboBox::drop-down { border: none; width: 16px; }
    QComboBox QAbstractItemView { background-color: #1e1e1e; color: #f0f0f0; selection-background-color: #2d6ba3; selection-color: #ffffff; }
    QCheckBox { color: #f0f0f0; font-size: 11px; spacing: 4px; }

    QTabWidget::pane { border: 1px solid #4a4a4a; background: #323232; }
    QTabBar::tab { background: #2b2b2b; color: #f0f0f0; padding: 5px 12px; border: 1px solid #4a4a4a; border-bottom: none; }
    QTabBar::tab:selected { background: #323232; }

    QStatusBar { background-color: #2b2b2b; color: #d0d0d0; border-top: 1px solid #1e1e1e; font-size: 11px; }
    QStatusBar QLabel { color: #d0d0d0; padding: 0 3px; }
    QStatusBar::item { border: none; }

    QScrollBar:vertical { background: #2b2b2b; width: 14px; margin: 0; }
    QScrollBar::handle:vertical { background: #555555; border-radius: 3px; min-height: 24px; }
    QScrollBar::handle:vertical:hover { background: #6a6a6a; }
    QScrollBar:horizontal { background: #2b2b2b; height: 14px; margin: 0; }
    QScrollBar::handle:horizontal { background: #555555; border-radius: 3px; min-width: 24px; }
    QScrollBar::handle:horizontal:hover { background: #6a6a6a; }
    QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)";

} // namespace

namespace Theme {

void setScheme(Scheme s) { g_scheme = s; }
Scheme scheme() { return g_scheme; }

bool isDark() {
    switch (g_scheme) {
    case Scheme::Light: return false;
    case Scheme::Dark:  return true;
    case Scheme::Default:
    default:            return osPrefersDark();
    }
}

QString styleSheet() {
    return QString::fromUtf8(isDark() ? kDark : kLight);
}

QString canvasBackdrop() {
    // paint.net surrounds the image with a mid gray in light mode and a much
    // darker gray in dark mode.
    return isDark() ? "#3c3c3c" : "#969696";
}

void loadFromSettings() {
    QSettings s("PaintDali", "PaintDali");
    const QString v = s.value("ui/colorScheme", "default").toString();
    if (v == "light") g_scheme = Scheme::Light;
    else if (v == "dark") g_scheme = Scheme::Dark;
    else g_scheme = Scheme::Default;
}

void saveToSettings() {
    QSettings s("PaintDali", "PaintDali");
    const char *v = (g_scheme == Scheme::Light) ? "light"
                  : (g_scheme == Scheme::Dark)  ? "dark" : "default";
    s.setValue("ui/colorScheme", v);
}

} // namespace Theme
