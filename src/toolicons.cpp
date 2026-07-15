#include "toolicons.h"
#include "theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>
#include <cmath>

namespace {

static const int TS = 20; // Tool icon size
static const int TBS = 16; // Toolbar icon size

QPixmap mkpm(int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    return pm;
}

// The icons are line art drawn in dark tones, which disappears against the dark
// colour scheme. On the dark theme we LIGHTEN every dark pixel (keeping its hue
// and saturation) so nothing stays invisible — neutral blacks become near-white,
// dark browns/blues become light browns/blues, while already-light pixels are
// left alone.
QIcon themed(const QPixmap &pm) {
    if (!Theme::isDark())
        return QIcon(pm);

    QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < img.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const int a = qAlpha(line[x]);
            if (a == 0) continue;
            int h, s, l, al;
            QColor c = QColor::fromRgb(line[x]);
            c.getHsl(&h, &s, &l, &al);
            if (l < 150) {
                // Reflect dark lightness up towards white (dark -> light) while
                // keeping the hue; the darker it was, the lighter it becomes.
                const int nl = qBound(0, 235 - l, 255);
                c.setHsl(h < 0 ? 0 : h, s, nl, a);
                line[x] = c.rgba();
            }
        }
    }
    return QIcon(QPixmap::fromImage(img));
}

void aa(QPainter &p) {
    p.setRenderHint(QPainter::Antialiasing, true);
}

// ==================== TOOL ICONS (20x20) ====================

QPixmap drawRectSelect() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPen pen(QColor(40, 40, 40), 1.4, Qt::DashLine);
    QVector<qreal> d; d << 3 << 2;
    pen.setDashPattern(d);
    p.setPen(pen);
    p.setBrush(QColor(100, 160, 255, 25));
    p.drawRect(3, 3, 13, 13);
    p.end();
    return pm;
}

QPixmap drawEllipseSelect() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPen pen(QColor(40, 40, 40), 1.4, Qt::DashLine);
    QVector<qreal> d; d << 3 << 2;
    pen.setDashPattern(d);
    p.setPen(pen);
    p.setBrush(QColor(100, 160, 255, 25));
    p.drawEllipse(3, 3, 13, 13);
    p.end();
    return pm;
}

QPixmap drawMagicWand() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Shaft
    p.setPen(QPen(QColor(160, 130, 50), 2.5));
    p.drawLine(2, 17, 11, 8);
    // Tip
    p.setPen(QPen(QColor(80, 60, 20), 2));
    p.drawLine(11, 8, 13, 6);
    // Sparkles
    p.setPen(QPen(QColor(255, 215, 0), 1.5));
    p.drawLine(15, 1, 15, 5);
    p.drawLine(13, 3, 17, 3);
    p.setPen(QPen(QColor(255, 240, 80), 1));
    p.drawLine(18, 5, 18, 7);
    p.drawLine(17, 6, 19, 6);
    p.drawPoint(11, 2);
    p.end();
    return pm;
}

QPixmap drawMove() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    int c = 10;
    p.setPen(QPen(QColor(40, 40, 40), 1.4));
    p.drawLine(c, 4, c, 16);
    p.drawLine(4, c, 16, c);
    p.setBrush(QColor(40, 40, 40));
    p.setPen(Qt::NoPen);
    QPointF u[] = {QPointF(c, 2), QPointF(c-3, 6), QPointF(c+3, 6)};
    p.drawPolygon(u, 3);
    QPointF dw[] = {QPointF(c, 18), QPointF(c-3, 14), QPointF(c+3, 14)};
    p.drawPolygon(dw, 3);
    QPointF le[] = {QPointF(2, c), QPointF(6, c-3), QPointF(6, c+3)};
    p.drawPolygon(le, 3);
    QPointF ri[] = {QPointF(18, c), QPointF(14, c-3), QPointF(14, c+3)};
    p.drawPolygon(ri, 3);
    p.end();
    return pm;
}

QPixmap drawBrush() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Handle
    p.setPen(QPen(QColor(100, 70, 30), 1));
    p.setBrush(QColor(175, 135, 75));
    QPainterPath h;
    h.moveTo(17, 1); h.lineTo(19, 3); h.lineTo(10, 12); h.lineTo(8, 10); h.closeSubpath();
    p.drawPath(h);
    // Ferrule
    p.setBrush(QColor(175, 175, 185));
    QPainterPath f;
    f.moveTo(10, 12); f.lineTo(8, 10); f.lineTo(6, 12); f.lineTo(8, 14); f.closeSubpath();
    p.drawPath(f);
    // Bristle tip
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(55, 115, 215));
    QPainterPath b;
    b.moveTo(6, 12); b.lineTo(8, 14); b.lineTo(3, 19); b.lineTo(1, 17); b.closeSubpath();
    p.drawPath(b);
    p.end();
    return pm;
}

QPixmap drawPencil() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Body (yellow)
    p.setPen(QPen(QColor(80, 80, 80), 0.8));
    p.setBrush(QColor(255, 215, 50));
    QPainterPath body;
    body.moveTo(16, 1); body.lineTo(18, 3); body.lineTo(7, 14); body.lineTo(5, 12); body.closeSubpath();
    p.drawPath(body);
    // Graphite tip
    p.setBrush(QColor(60, 60, 60));
    QPainterPath tip;
    tip.moveTo(7, 14); tip.lineTo(5, 12); tip.lineTo(2, 18); tip.closeSubpath();
    p.drawPath(tip);
    // Pink eraser top
    p.setBrush(QColor(240, 150, 170));
    QPainterPath er;
    er.moveTo(16, 1); er.lineTo(18, 3); er.lineTo(17, 4); er.lineTo(15, 2); er.closeSubpath();
    p.drawPath(er);
    p.end();
    return pm;
}

QPixmap drawEraser() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(80, 60, 60), 1));
    // Main eraser body (pink)
    p.setBrush(QColor(245, 155, 175));
    QPainterPath body;
    body.moveTo(15, 3); body.lineTo(18, 6); body.lineTo(8, 16); body.lineTo(5, 13); body.closeSubpath();
    p.drawPath(body);
    // White band
    p.setBrush(QColor(255, 225, 235));
    QPainterPath band;
    band.moveTo(8, 16); band.lineTo(5, 13); band.lineTo(2, 16); band.lineTo(5, 19); band.closeSubpath();
    p.drawPath(band);
    p.end();
    return pm;
}

QPixmap drawFill() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Bucket body
    p.setPen(QPen(QColor(70, 70, 70), 1.2));
    p.setBrush(QColor(190, 190, 195));
    QPainterPath bucket;
    bucket.moveTo(4, 7); bucket.lineTo(4, 15); bucket.lineTo(13, 15); bucket.lineTo(13, 7); bucket.closeSubpath();
    p.drawPath(bucket);
    // Handle
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRect(3, 3, 8, 6), 30*16, 120*16);
    // Paint pouring
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(55, 115, 215));
    QPainterPath paint;
    paint.moveTo(13, 8); paint.quadTo(16, 10, 17, 16); paint.lineTo(15, 16);
    paint.quadTo(14, 11, 13, 10); paint.closeSubpath();
    p.drawPath(paint);
    p.end();
    return pm;
}

QPixmap drawColorPicker() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Bulb top
    p.setPen(QPen(QColor(70, 70, 70), 1));
    p.setBrush(QColor(210, 210, 220));
    p.drawEllipse(10, 1, 8, 8);
    // Shaft
    p.setBrush(QColor(190, 190, 200));
    QPainterPath shaft;
    shaft.moveTo(12, 7); shaft.lineTo(15, 7); shaft.lineTo(7, 15); shaft.lineTo(5, 13); shaft.closeSubpath();
    p.drawPath(shaft);
    // Tip
    p.setBrush(QColor(50, 50, 50));
    QPainterPath tip;
    tip.moveTo(7, 15); tip.lineTo(5, 13); tip.lineTo(2, 18); tip.closeSubpath();
    p.drawPath(tip);
    p.end();
    return pm;
}

QPixmap drawRecolor() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Small brush
    p.setPen(QPen(QColor(80, 80, 80), 1.2));
    p.drawLine(16, 2, 8, 10);
    // Two overlapping color circles
    p.setPen(QPen(QColor(60, 60, 60), 0.8));
    p.setBrush(QColor(220, 60, 60));
    p.drawEllipse(1, 9, 8, 8);
    p.setBrush(QColor(60, 60, 220));
    p.drawEllipse(6, 12, 8, 8);
    // Circular arrow hint
    p.setPen(QPen(QColor(40, 40, 40), 1.3));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRect(3, 10, 10, 10), 60*16, 180*16);
    p.end();
    return pm;
}

QPixmap drawCloneStamp() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Stamp head (circular)
    p.setPen(QPen(QColor(70, 70, 70), 1.2));
    p.setBrush(QColor(170, 170, 175));
    p.drawRoundedRect(4, 13, 12, 5, 2, 2);
    // Handle
    p.setBrush(QColor(140, 100, 60));
    p.drawRect(8, 4, 4, 9);
    // Handle top
    p.setBrush(QColor(100, 70, 40));
    p.drawRoundedRect(6, 1, 8, 4, 1, 1);
    p.end();
    return pm;
}

QPixmap drawText() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QFont font;
    font.setPixelSize(16);
    font.setBold(true);
    font.setFamily("Sans");
    p.setFont(font);
    p.setPen(QColor(30, 30, 30));
    p.drawText(QRect(0, 0, TS, TS), Qt::AlignCenter, "A");
    // Small cursor line
    p.setPen(QPen(QColor(50, 100, 200), 1));
    p.drawLine(15, 3, 15, 17);
    p.end();
    return pm;
}

QPixmap drawLine() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(40, 40, 40), 2));
    p.drawLine(3, 17, 17, 3);
    // Endpoint dots
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(50, 100, 200));
    p.drawEllipse(QPointF(3, 17), 2.5, 2.5);
    p.drawEllipse(QPointF(17, 3), 2.5, 2.5);
    p.end();
    return pm;
}

QPixmap drawShape() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(50, 100, 200), 1.5));
    p.setBrush(QColor(50, 100, 200, 30));
    p.drawRoundedRect(2, 4, 15, 12, 2, 2);
    p.end();
    return pm;
}

QPixmap drawGradient() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient grad(3, 0, 17, 0);
    grad.setColorAt(0, Qt::black);
    grad.setColorAt(1, Qt::white);
    p.setPen(QPen(QColor(70, 70, 70), 1));
    p.setBrush(grad);
    p.drawRect(3, 4, 14, 12);
    p.end();
    return pm;
}

QPixmap drawLasso() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPen pen(QColor(40, 40, 40), 1.4, Qt::DashLine);
    QVector<qreal> d; d << 3 << 2;
    pen.setDashPattern(d);
    p.setPen(pen);
    p.setBrush(QColor(100, 160, 255, 25));
    // Freehand lasso shape
    QPainterPath path;
    path.moveTo(9, 3);
    path.cubicTo(16, 2, 18, 8, 15, 12);
    path.cubicTo(13, 16, 8, 17, 5, 14);
    path.cubicTo(1, 11, 2, 5, 5, 4);
    path.cubicTo(6, 3.5, 7, 3, 9, 3);
    p.drawPath(path);
    p.end();
    return pm;
}

QPixmap drawMoveSelection() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Dashed selection rect
    QPen dashPen(QColor(40, 40, 40), 1.4, Qt::DashLine);
    QVector<qreal> d; d << 3 << 2;
    dashPen.setDashPattern(d);
    p.setPen(dashPen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(2, 2, 12, 12);
    // 4-directional move arrow smaller, inside
    int c = 8;
    p.setPen(QPen(QColor(40, 40, 100), 1.2));
    p.drawLine(c, 5, c, 11);
    p.drawLine(5, c, 11, c);
    p.setBrush(QColor(40, 40, 100));
    p.setPen(Qt::NoPen);
    QPointF u[] = {QPointF(c,3.5), QPointF(c-2,6), QPointF(c+2,6)};
    p.drawPolygon(u, 3);
    QPointF dw[] = {QPointF(c,12.5), QPointF(c-2,10), QPointF(c+2,10)};
    p.drawPolygon(dw, 3);
    QPointF le[] = {QPointF(3.5,c), QPointF(6,c-2), QPointF(6,c+2)};
    p.drawPolygon(le, 3);
    QPointF ri[] = {QPointF(12.5,c), QPointF(10,c-2), QPointF(10,c+2)};
    p.drawPolygon(ri, 3);
    p.end();
    return pm;
}

QPixmap drawZoom() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Magnifier glass
    p.setPen(QPen(QColor(60, 60, 60), 2));
    p.setBrush(QColor(200, 225, 255, 100));
    p.drawEllipse(3, 3, 11, 11);
    // Handle
    p.setPen(QPen(QColor(60, 60, 60), 2.5));
    p.drawLine(13, 13, 17, 17);
    // + sign inside
    p.setPen(QPen(QColor(40, 40, 40), 1.5));
    p.drawLine(7, 8, 10, 8);
    p.drawLine(8, 6, 8, 10);
    p.end();
    return pm;
}

QPixmap drawPan() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Clean open "grab" hand. Drawn in neutral ink so themed() flips it light on
    // the dark scheme.
    const QColor ink(70, 70, 70);
    const QColor skin(238, 205, 170);
    p.setPen(QPen(ink, 1.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(skin);

    QPainterPath hand;
    // Palm + wrist as one rounded body.
    hand.addRoundedRect(QRectF(5.5, 8.5, 9.0, 9.5), 3.0, 3.0);
    p.drawPath(hand);

    // Four fingers (rounded bars) rising from the palm.
    auto finger = [&](double x, double topY) {
        p.drawRoundedRect(QRectF(x, topY, 2.0, 8.0 - (topY - 4.0) * 0.0), 1.0, 1.0);
    };
    p.setBrush(skin);
    p.drawRoundedRect(QRectF(6.2, 3.2, 2.0, 7.0), 1.0, 1.0);
    p.drawRoundedRect(QRectF(8.6, 2.4, 2.0, 8.0), 1.0, 1.0);
    p.drawRoundedRect(QRectF(11.0, 3.0, 2.0, 7.4), 1.0, 1.0);
    p.drawRoundedRect(QRectF(13.2, 4.6, 2.0, 6.0), 1.0, 1.0);
    // Thumb on the left.
    p.drawRoundedRect(QRectF(3.2, 9.0, 3.4, 2.1), 1.0, 1.0);
    (void)finger;
    p.end();
    return pm;
}

// ==================== TOOLBAR ICONS (16x16) ====================

QPixmap drawNewDoc() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    // Page
    p.setPen(QPen(QColor(100, 100, 100), 1));
    p.setBrush(Qt::white);
    QPainterPath page;
    page.moveTo(3, 1); page.lineTo(10, 1); page.lineTo(13, 4); page.lineTo(13, 15); page.lineTo(3, 15); page.closeSubpath();
    p.drawPath(page);
    // Corner fold
    p.setBrush(QColor(200, 210, 230));
    QPainterPath fold;
    fold.moveTo(10, 1); fold.lineTo(10, 4); fold.lineTo(13, 4); fold.closeSubpath();
    p.drawPath(fold);
    p.end();
    return pm;
}

QPixmap drawOpenDoc() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    // Folder back
    p.setPen(QPen(QColor(150, 120, 30), 1));
    p.setBrush(QColor(230, 190, 60));
    p.drawRoundedRect(1, 3, 14, 11, 1, 1);
    // Tab
    p.drawRoundedRect(1, 1, 6, 3, 1, 1);
    // Open front
    p.setBrush(QColor(250, 215, 80));
    QPainterPath front;
    front.moveTo(1, 7); front.lineTo(3, 14); front.lineTo(15, 14); front.lineTo(15, 7); front.closeSubpath();
    p.drawPath(front);
    p.end();
    return pm;
}

QPixmap drawSaveDoc() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    // Floppy disk body
    p.setPen(QPen(QColor(30, 60, 120), 1));
    p.setBrush(QColor(70, 130, 210));
    p.drawRoundedRect(1, 1, 14, 14, 1, 1);
    // Metal shutter
    p.setBrush(QColor(180, 185, 195));
    p.drawRect(4, 1, 8, 5);
    // Shutter slot
    p.setBrush(QColor(70, 130, 210));
    p.drawRect(9, 1, 2, 4);
    // Label area
    p.setBrush(Qt::white);
    p.drawRect(3, 9, 10, 5);
    // Label lines
    p.setPen(QPen(QColor(150, 150, 150), 0.5));
    p.drawLine(5, 11, 11, 11);
    p.drawLine(5, 13, 11, 13);
    p.end();
    return pm;
}

QPixmap drawUndo() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(50, 100, 200), 2));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRect(3, 4, 10, 10), 45*16, 225*16);
    // Arrow head
    p.setBrush(QColor(50, 100, 200));
    p.setPen(Qt::NoPen);
    QPointF arr[] = {QPointF(3, 4), QPointF(7, 3), QPointF(5, 7)};
    p.drawPolygon(arr, 3);
    p.end();
    return pm;
}

QPixmap drawRedo() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(50, 100, 200), 2));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRect(3, 4, 10, 10), -45*16, -225*16);
    // Arrow head
    p.setBrush(QColor(50, 100, 200));
    p.setPen(Qt::NoPen);
    QPointF arr[] = {QPointF(13, 4), QPointF(9, 3), QPointF(11, 7)};
    p.drawPolygon(arr, 3);
    p.end();
    return pm;
}

QPixmap drawCut() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(60, 60, 60), 1.3));
    p.setBrush(Qt::NoBrush);
    // Left blade
    p.drawEllipse(1, 10, 5, 5);
    p.drawLine(4, 11, 10, 3);
    // Right blade
    p.drawEllipse(10, 10, 5, 5);
    p.drawLine(12, 11, 6, 3);
    // Pivot
    p.setBrush(QColor(60, 60, 60));
    p.drawEllipse(QPointF(8, 7), 1.2, 1.2);
    p.end();
    return pm;
}

QPixmap drawCopy() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(100, 100, 100), 1));
    // Back page
    p.setBrush(QColor(220, 225, 235));
    p.drawRect(4, 1, 11, 11);
    // Front page
    p.setBrush(Qt::white);
    p.drawRect(1, 4, 11, 11);
    // Lines on front
    p.setPen(QPen(QColor(170, 170, 170), 0.7));
    p.drawLine(3, 8, 10, 8);
    p.drawLine(3, 10, 10, 10);
    p.drawLine(3, 12, 8, 12);
    p.end();
    return pm;
}

QPixmap drawPaste() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    // Clipboard
    p.setPen(QPen(QColor(140, 110, 50), 1));
    p.setBrush(QColor(210, 180, 100));
    p.drawRoundedRect(1, 2, 11, 13, 1, 1);
    // Clip
    p.setBrush(QColor(160, 140, 80));
    p.drawRoundedRect(4, 1, 5, 3, 1, 1);
    // Page on clipboard
    p.setPen(QPen(QColor(100, 100, 100), 0.8));
    p.setBrush(Qt::white);
    p.drawRect(3, 5, 8, 8);
    // Lines
    p.setPen(QPen(QColor(170, 170, 170), 0.7));
    p.drawLine(5, 7, 9, 7);
    p.drawLine(5, 9, 9, 9);
    p.end();
    return pm;
}

} // anonymous namespace

namespace ToolIcons {

QIcon forTool(ToolType type) {
    switch (type) {
    case ToolType::RectSelection:   return themed(drawRectSelect());
    case ToolType::EllipseSelection:return themed(drawEllipseSelect());
    case ToolType::LassoSelection:  return themed(drawLasso());
    case ToolType::MagicWand:       return themed(drawMagicWand());
    case ToolType::Move:            return themed(drawMove());
    case ToolType::MoveSelection:   return themed(drawMoveSelection());
    case ToolType::Zoom:            return themed(drawZoom());
    case ToolType::Pan:             return themed(drawPan());
    case ToolType::Brush:           return themed(drawBrush());
    case ToolType::Pencil:          return themed(drawPencil());
    case ToolType::Eraser:          return themed(drawEraser());
    case ToolType::Fill:            return themed(drawFill());
    case ToolType::ColorPicker:     return themed(drawColorPicker());
    case ToolType::Recolor:         return themed(drawRecolor());
    case ToolType::CloneStamp:      return themed(drawCloneStamp());
    case ToolType::Text:            return themed(drawText());
    case ToolType::Line:            return themed(drawLine());
    case ToolType::Shape:           return themed(drawShape());
    case ToolType::Gradient:        return themed(drawGradient());
    default: return QIcon();
    }
}

QIcon newDoc() { return themed(drawNewDoc()); }
QIcon openDoc() { return themed(drawOpenDoc()); }
QIcon saveDoc() { return themed(drawSaveDoc()); }
QIcon undoAction() { return themed(drawUndo()); }
QIcon redoAction() { return themed(drawRedo()); }
QIcon cutAction() { return themed(drawCut()); }
QIcon copyAction() { return themed(drawCopy()); }
QIcon pasteAction() { return themed(drawPaste()); }

// ==================== TOOLBAR / MENUBAR EXTRAS (16x16) ====================

QIcon printAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(80, 80, 90), 1));
    p.setBrush(QColor(235, 238, 242));
    p.drawRect(4, 2, 8, 4);            // paper out top
    p.setBrush(QColor(120, 130, 145));
    p.drawRoundedRect(2, 6, 12, 5, 1, 1);  // printer body
    p.setBrush(Qt::white);
    p.drawRect(4, 10, 8, 4);           // printed sheet
    p.end();
    return themed(pm);
}

QIcon cropAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(60, 60, 70), 1.4));
    p.drawLine(4, 1, 4, 12);
    p.drawLine(1, 11, 12, 11);
    p.setPen(QPen(QColor(60, 60, 70), 1.4, Qt::DashLine));
    p.drawLine(11, 4, 11, 15);
    p.drawLine(4, 4, 15, 4);
    p.end();
    return themed(pm);
}

QIcon deselectAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(110, 110, 120), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(2, 2, 11, 11);
    p.setPen(QPen(QColor(200, 60, 60), 1.6));
    p.drawLine(5, 5, 11, 11);
    p.drawLine(11, 5, 5, 11);
    p.end();
    return themed(pm);
}

QIcon pixelGridAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    p.setPen(QPen(QColor(110, 120, 135), 1));
    for (int i = 1; i <= 13; i += 4) {
        p.drawLine(i, 1, i, 14);
        p.drawLine(1, i, 14, i);
    }
    p.end();
    return themed(pm);
}

QIcon rulersAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(90, 90, 100), 1));
    p.setBrush(QColor(250, 244, 200));
    p.drawRect(1, 5, 14, 6);
    p.setPen(QPen(QColor(90, 90, 100), 1));
    for (int x = 3; x < 15; x += 3) p.drawLine(x, 5, x, 8);
    p.end();
    return themed(pm);
}

// ---- Utility-window icons (menu bar, right side) ----

QIcon toolsWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(70, 120, 185), 1.2));
    p.setBrush(QColor(70, 120, 185));
    p.drawRect(2, 11, 12, 2);
    p.drawRect(7, 3, 2, 8);
    p.drawRect(5, 2, 6, 2);
    p.end();
    return themed(pm);
}

QIcon historyWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(70, 120, 185), 1.4));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(2, 2, 12, 12);
    p.drawLine(8, 4, 8, 8);
    p.drawLine(8, 8, 11, 9);
    p.end();
    return themed(pm);
}

QIcon layersWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(70, 120, 185), 1));
    p.setBrush(QColor(180, 210, 235));
    p.drawRect(3, 3, 9, 6);
    p.setBrush(QColor(140, 185, 220));
    p.drawRect(5, 7, 9, 6);
    p.end();
    return themed(pm);
}

QIcon colorsWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient g(2, 2, 14, 14);
    g.setColorAt(0.0, QColor(255, 80, 80));
    g.setColorAt(0.2, QColor(255, 210, 80));
    g.setColorAt(0.4, QColor(80, 220, 80));
    g.setColorAt(0.6, QColor(80, 220, 220));
    g.setColorAt(0.8, QColor(80, 120, 255));
    g.setColorAt(1.0, QColor(200, 80, 255));
    p.setPen(QPen(QColor(90, 90, 100), 1));
    p.setBrush(g);
    p.drawEllipse(2, 2, 12, 12);
    p.end();
    return themed(pm);
}

QIcon settings() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(90, 95, 105), 1.3));
    p.setBrush(QColor(200, 206, 214));
    // simple gear: circle + teeth
    p.drawEllipse(4, 4, 8, 8);
    p.setPen(QPen(QColor(90, 95, 105), 1.6));
    for (int a = 0; a < 360; a += 45) {
        double r = a * M_PI / 180.0;
        p.drawLine(QPointF(8 + 5.0 * cos(r), 8 + 5.0 * sin(r)),
                   QPointF(8 + 7.0 * cos(r), 8 + 7.0 * sin(r)));
    }
    p.setBrush(Qt::white);
    p.setPen(QPen(QColor(90, 95, 105), 1));
    p.drawEllipse(6, 6, 4, 4);
    p.end();
    return themed(pm);
}

QIcon help() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(70, 120, 185), 1.3));
    p.setBrush(QColor(225, 238, 250));
    p.drawEllipse(1, 1, 14, 14);
    QFont f("Sans", 9, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(40, 90, 160));
    p.drawText(QRect(1, 1, 14, 14), Qt::AlignCenter, "?");
    p.end();
    return themed(pm);
}

} // namespace ToolIcons
