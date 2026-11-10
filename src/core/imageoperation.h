#pragma once
#include "selection.h"
#include <functional>
using ImageOperation = std::function<QImage(const QImage &)>;
// Pure, size-preserving operation in layer coordinates; preserves selection coverage.
QImage adjustedImage(const QImage &source, const QPoint &offset,
                     const Selection &selection, const ImageOperation &operation);

// Layers > Rotate / Zoom: rotate by `angleDegrees` and scale by `scale` about the
// image centre, keeping the image size (pixels pushed outside are cropped).
QImage rotateZoomImage(const QImage &source, double angleDegrees, double scale);
