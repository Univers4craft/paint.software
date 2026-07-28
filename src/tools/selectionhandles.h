#pragma once

#include <QRect>
#include <QPointF>
#include <QVector>
#include <QImage>
#include <QTransform>
#include <QtGlobal>

class Selection;

// Geometry shared by the two selection-manipulating tools: Move Selected Pixels
// resizes the selected artwork, Move Selection resizes only the marquee, but the
// handles must look and grab identically in both.
namespace SelHandles {

enum class Handle { None, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left };

// Half-size of a handle, in canvas units: a screen-space size divided by the
// zoom, so it keeps a constant on-screen size, capped at a quarter of the
// selection. The cap matters — with a flat canvas-space radius the corner zones
// of a small selection met in the middle and swallowed every press, leaving no
// way to drag the selection itself.
double radius(const QRect &rect, double zoom);

// The handle under `canvasPos`, or None. Drawing and hit-testing share radius(),
// so the handle you see is the handle you can grab.
Handle at(const QRect &rect, const QPointF &canvasPos, double zoom);

// `rect` with the given handle dragged to `canvasPos`, normalized, never smaller
// than 2x2 (a zero-size rect would make the scaled image collapse).
QRect resized(const QRect &rect, Handle handle, const QPointF &canvasPos);

// As above, but honouring Paint.NET's resize modifiers: Shift keeps the original
// aspect ratio (corner handles), Alt resizes symmetrically about the centre,
// Shift+Alt does both. With no modifier this is identical to the overload above,
// so the un-modified drag keeps its exact pixel-for-pixel behaviour.
QRect resized(const QRect &rect, Handle handle, const QPointF &canvasPos,
              Qt::KeyboardModifiers mods);

// True when `canvasPos` sits in the rotation "corridor": just outside the
// bounding box (past the resize handles) but within a screen-space band of it.
// Dragging there rotates the content, like Paint.NET.
bool inRotationZone(const QRect &rect, const QPointF &canvasPos, double zoom);

// A rotation of `angleDeg` (clockwise, degrees) about `center`, in canvas space.
QTransform rotationAbout(const QPointF &center, double angleDeg);

// The eight handle squares, in canvas units, for drawing.
QVector<QRectF> rects(const QRect &rect, double zoom);

// Rebuild `sel` by scaling the part of `originalMask` inside `from` onto `to`.
// Used both live while dragging and when committing.
void setScaledMask(Selection &sel, const QImage &originalMask,
                   const QRect &from, const QRect &to, bool smooth);

// Rebuild `sel` by rotating `originalMask` by `angleDeg` about `center`. Used
// while dragging a rotation and when committing it.
void setRotatedMask(Selection &sel, const QImage &originalMask,
                    const QPointF &center, double angleDeg, bool smooth);

} // namespace SelHandles
