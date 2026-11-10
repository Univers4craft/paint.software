#include <QPainter>
#include "imageoperation.h"
QImage adjustedImage(const QImage &source, const QPoint &offset,
                     const Selection &selection, const ImageOperation &operation) {
    if (!selection.hasSelection()) {
        QImage result = operation(source);
        return result.size() == source.size() ? result.convertToFormat(source.format()) : source;
    }
    QRect rect = selection.boundingRect().translated(-offset).intersected(source.rect());
    if (rect.isEmpty()) return source;
    QImage original = source.copy(rect).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage processed = operation(original).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (processed.size() != original.size()) return source;
    // Coverage blending interpolates premultiplied colour and alpha together.
    // The adjustment itself (e.g. Curves) un-premultiplies before colour math.
    QImage result = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage mask = selection.maskForImage(source.size(), offset).copy(rect).convertToFormat(QImage::Format_Grayscale8);
    for (int y = 0; y < rect.height(); ++y) {
        auto *dest = reinterpret_cast<QRgb *>(result.scanLine(rect.y()+y)) + rect.x();
        const auto *src = reinterpret_cast<const QRgb *>(original.constScanLine(y));
        const auto *fx = reinterpret_cast<const QRgb *>(processed.constScanLine(y));
        const auto *coverage = mask.constScanLine(y);
        for (int x = 0; x < rect.width(); ++x) {
            int a = coverage[x], inv = 255-a;
            if (!a) continue;
            dest[x] = qRgba((qRed(src[x])*inv+qRed(fx[x])*a)/255,
                            (qGreen(src[x])*inv+qGreen(fx[x])*a)/255,
                            (qBlue(src[x])*inv+qBlue(fx[x])*a)/255,
                            (qAlpha(src[x])*inv+qAlpha(fx[x])*a)/255);
        }
    }
    return result.convertToFormat(source.format());
}

QImage rotateZoomImage(const QImage &source, double angleDegrees, double scale) {
    if (angleDegrees == 0.0 && scale == 1.0) return source;
    const QSize sz = source.size();
    QImage result(sz, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.translate(sz.width() / 2.0, sz.height() / 2.0);
    p.rotate(angleDegrees);
    p.scale(scale, scale);
    p.translate(-sz.width() / 2.0, -sz.height() / 2.0);
    p.drawImage(0, 0, source);
    p.end();
    return result;
}
