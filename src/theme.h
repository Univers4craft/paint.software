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

void loadFromSettings();
void saveToSettings();

} // namespace Theme
