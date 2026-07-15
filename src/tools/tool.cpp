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
