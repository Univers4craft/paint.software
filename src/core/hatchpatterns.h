#pragma once

#include <QString>
#include <QBrush>
#include <QColor>

// Paint.NET's "Fill Style" is a dropdown of a solid fill plus the full GDI+
// HatchStyle set (~53 patterns). This reproduces that list: index 0 is Solid,
// indices 1..53 are the hatch patterns, in GDI+ enum order so the dropdown
// reads like Paint.NET's.
namespace Hatch {

// Number of fill styles, including Solid at index 0.
int count();

// Human-readable name of a fill style (English source; goes through TR()).
QString name(int index);

// A brush that paints fill style `index` using `fg` for the pattern lines/dots
// and `bg` for the background. Index 0 (or out of range) returns a solid `fg`
// brush. The pattern tiles seamlessly.
QBrush brush(int index, const QColor &fg, const QColor &bg);

}  // namespace Hatch
