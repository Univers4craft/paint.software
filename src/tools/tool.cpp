#include "tool.h"
#include "core/document.h"
#include "core/selection.h"
#include <QPainter>
#include <QImage>

void Tool::clipToSelection(QPainter &painter, Document *doc) {
    if (!doc) return;
    const Selection &sel = doc->selection();
    if (!sel.hasSelection()) return;
    QRegion region = sel.region();
    if (!region.isEmpty())
        painter.setClipRegion(region);
}

QPainter::CompositionMode Tool::compositionModeFor(int blendIndex) {
    // Maps the blend-mode combo (Layer::allBlendModes order) to a Qt mode.
    switch (blendIndex) {
    case 1:  return QPainter::CompositionMode_Multiply;   // Multiply
    case 2:  return QPainter::CompositionMode_Plus;        // Additive
    case 3:  return QPainter::CompositionMode_ColorBurn;
    case 4:  return QPainter::CompositionMode_ColorDodge;
    case 5:  return QPainter::CompositionMode_Darken;
    case 6:  return QPainter::CompositionMode_Difference;
    case 7:  return QPainter::CompositionMode_ColorDodge;  // Glow    (approximation)
    case 8:  return QPainter::CompositionMode_Lighten;
    case 9:  return QPainter::CompositionMode_Difference;  // Negation (approximation)
    case 10: return QPainter::CompositionMode_Overlay;
    case 11: return QPainter::CompositionMode_Screen;      // Reflect (approximation)
    case 12: return QPainter::CompositionMode_Screen;
    case 13: return QPainter::CompositionMode_Xor;         // Xor
    case 14: return QPainter::CompositionMode_Source;      // Overwrite (callers replace per-pixel)
    default: return QPainter::CompositionMode_SourceOver;  // Normal + unsupported
    }
}

bool Tool::selectionAllows(Document *doc, int x, int y) {
    if (!doc) return true;
    const Selection &sel = doc->selection();
    if (!sel.hasSelection()) return true;
    return sel.isSelected(x, y);
}

QImage Tool::maskEditToSelection(Document *doc, const QImage &before, const QImage &after) {
    if (!doc) return after;
    const Selection &sel = doc->selection();
    if (!sel.hasSelection()) return after;

    QImage result = before.convertToFormat(QImage::Format_ARGB32);
    QImage src = after.convertToFormat(QImage::Format_ARGB32);
    const int w = result.width(), h = result.height();
    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        const QRgb *s = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            int cov = sel.selectionValue(x, y);
            if (cov == 0) continue;
            if (cov == 255) { dst[x] = s[x]; continue; }
            // Partial (feathered) coverage: blend before/after per channel.
            QRgb b = dst[x];
            int ia = 255 - cov;
            dst[x] = qRgba(
                (qRed(s[x]) * cov + qRed(b) * ia) / 255,
                (qGreen(s[x]) * cov + qGreen(b) * ia) / 255,
                (qBlue(s[x]) * cov + qBlue(b) * ia) / 255,
                (qAlpha(s[x]) * cov + qAlpha(b) * ia) / 255);
        }
    }
    return result.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}
