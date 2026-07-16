#pragma once

#include <QString>

// Lightweight UI translation layer. The source strings are French (the language
// the UI was written in); English is provided through a lookup table.
namespace I18n {

enum class Lang { French, English };

void setLanguage(Lang lang);
Lang language();

// Persisted in QSettings under "ui/language".
void loadFromSettings();
void saveToSettings();

// Loads Qt's own translations for the current language. Qt renders the standard
// dialog buttons (OK / Cancel / Yes / No / Save) itself, so without this they
// stay English inside an otherwise French UI. Call after setLanguage().
void applyQtTranslations();

// Translate a French UI string. Returns it unchanged when the language is
// French, or when no translation exists (so untranslated strings degrade
// gracefully rather than disappearing).
QString t(const QString &french);

} // namespace I18n

// Short alias used at call sites.
inline QString TR(const QString &french) { return I18n::t(french); }
