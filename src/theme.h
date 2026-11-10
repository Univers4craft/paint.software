#pragma once

#include <QString>

// paint.net's "Color Scheme" setting: Default follows the OS, Light and Dark
// force a scheme.
namespace Theme {

enum class Scheme { Default, Light, Dark };

void setScheme(Scheme s);
Scheme scheme();

// True when the effective scheme (after resolving Default against the OS) is dark.
bool isDark();

// The full application stylesheet for the effective scheme.
QString styleSheet();

// Canvas backdrop colour (around the image) for the effective scheme.
QString canvasBackdrop();

// Text size (issue #32). The stylesheets size text in pixels, which ignores the
// desktop's font size; on a high-resolution screen without display scaling the
// menus came out unreadably small. The text-size setting multiplies every
// `font-size: Npx` the app sets. 0 means automatic: follow the desktop font size
// (never smaller than the designed size); otherwise a percentage (100..200).
int fontScalePercent();
void setFontScalePercent(int percent);
// Effective multiplier (automatic resolved against the desktop font).
double fontScale();
// Pure helper: multiply every `font-size: Npx` in a stylesheet by `scale`.
QString scaleFontSizes(const QString &css, double scale);
// Use for every widget-local stylesheet that sets a font-size, so it follows the
// text-size setting like the global one does.
QString css(const QString &styleSheet);

void loadFromSettings();
void saveToSettings();

} // namespace Theme
