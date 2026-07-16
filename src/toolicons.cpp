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

// ---- Colourful, paint.net-style tool icons -------------------------------
// These are drawn in real colours (like paint.net's) so they read the same on a
// light or dark background — they deliberately bypass themed(), which exists only
// to rescue the older dark line-art icons.

// Small helper: the dashed "marching ants" outline used by the selection tools.
void marchingAnts(QPainter &p) {
    QPen pen(QColor(65, 75, 88), 1.2, Qt::DashLine);
    QVector<qreal> d; d << 2.5 << 1.8;
    pen.setDashPattern(d);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
}

QPixmap drawRectSelect() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(137, 190, 233));      // light blue plate
    p.drawRect(4, 4, 12, 12);
    marchingAnts(p);
    p.drawRect(4, 4, 12, 12);
    p.end();
    return pm;
}

QPixmap drawEllipseSelect() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(137, 190, 233));
    p.drawEllipse(3, 3, 14, 14);
    marchingAnts(p);
    p.drawEllipse(3, 3, 14, 14);
    p.end();
    return pm;
}

QPixmap drawMagicWand() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Slate shaft running bottom-left -> top-right, with a lighter tip.
    QPen shaft(QColor(74, 85, 101), 3.0);
    shaft.setCapStyle(Qt::RoundCap);
    p.setPen(shaft);
    p.drawLine(3, 17, 11, 9);
    QPen tip(QColor(196, 208, 222), 3.0);
    tip.setCapStyle(Qt::RoundCap);
    p.setPen(tip);
    p.drawLine(11, 9, 13, 7);
    // Golden sparkles around the tip.
    auto sparkle = [&](qreal cx, qreal cy, qreal r, const QColor &c, qreal w) {
        QPen sp(c, w); sp.setCapStyle(Qt::RoundCap);
        p.setPen(sp);
        p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));
        p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    };
    sparkle(15.5, 4.0, 3.2, QColor(255, 196, 36), 1.6);
    sparkle(9.5, 3.0, 1.9, QColor(255, 226, 120), 1.2);
    sparkle(18.0, 9.5, 1.7, QColor(255, 226, 120), 1.2);
    p.end();
    return pm;
}

// Move Selected Pixels: a FILLED blue arrow + a small 4-way move cross.
QPixmap drawMove() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPainterPath a;
    a.moveTo(2, 1); a.lineTo(2, 12.5); a.lineTo(5.0, 9.8);
    a.lineTo(7.0, 14.2); a.lineTo(8.9, 13.3); a.lineTo(7.0, 9.2);
    a.lineTo(11.0, 8.8); a.closeSubpath();
    p.setPen(QPen(QColor(32, 56, 92), 1.0));
    p.setBrush(QColor(96, 146, 205));
    p.drawPath(a);
    // Compact move cross, bottom-right.
    const qreal cx = 14.5, cy = 14.5, r = 5.0;
    QPen cp(QColor(64, 84, 110), 1.3); cp.setCapStyle(Qt::RoundCap);
    p.setPen(cp);
    p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));
    p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(64, 84, 110));
    const qreal h = 2.0;
    QPointF up[] = {QPointF(cx, cy - r - 0.8), QPointF(cx - h, cy - r + 1.6), QPointF(cx + h, cy - r + 1.6)};
    QPointF dn[] = {QPointF(cx, cy + r + 0.8), QPointF(cx - h, cy + r - 1.6), QPointF(cx + h, cy + r - 1.6)};
    QPointF lf[] = {QPointF(cx - r - 0.8, cy), QPointF(cx - r + 1.6, cy - h), QPointF(cx - r + 1.6, cy + h)};
    QPointF rt[] = {QPointF(cx + r + 0.8, cy), QPointF(cx + r - 1.6, cy - h), QPointF(cx + r - 1.6, cy + h)};
    p.drawPolygon(up, 3); p.drawPolygon(dn, 3); p.drawPolygon(lf, 3); p.drawPolygon(rt, 3);
    p.end();
    return pm;
}

// paint.net's paintbrush: blue handle, steel ferrule, blue bristle point.
QPixmap drawBrush() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient hg(18, 1, 8, 11);
    hg.setColorAt(0.0, QColor(126, 180, 238));
    hg.setColorAt(1.0, QColor(54, 106, 178));
    p.setPen(QPen(QColor(28, 58, 98), 0.9));
    p.setBrush(hg);
    QPainterPath h;
    h.moveTo(17.4, 1.2); h.lineTo(19.0, 2.8); h.lineTo(10.2, 11.6); h.lineTo(8.4, 9.8);
    h.closeSubpath();
    p.drawPath(h);
    // Ferrule
    p.setPen(QPen(QColor(92, 98, 110), 0.9));
    p.setBrush(QColor(198, 205, 216));
    QPainterPath f;
    f.moveTo(10.2, 11.6); f.lineTo(8.4, 9.8); f.lineTo(6.2, 12.0); f.lineTo(8.0, 13.8);
    f.closeSubpath();
    p.drawPath(f);
    // Bristles
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(40, 92, 170));
    QPainterPath b;
    b.moveTo(6.2, 12.0); b.lineTo(8.0, 13.8); b.lineTo(1.8, 18.6);
    b.closeSubpath();
    p.drawPath(b);
    p.end();
    return pm;
}

// paint.net's pencil: warm amber body, wood collar, graphite point.
QPixmap drawPencil() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient bg(16, 2, 7, 12);
    bg.setColorAt(0.0, QColor(255, 208, 88));
    bg.setColorAt(1.0, QColor(236, 158, 40));
    p.setPen(QPen(QColor(122, 78, 20), 0.9));
    p.setBrush(bg);
    QPainterPath body;
    body.moveTo(16.2, 1.4); body.lineTo(18.2, 3.4); body.lineTo(7.6, 13.6); body.lineTo(5.6, 11.6);
    body.closeSubpath();
    p.drawPath(body);
    // Wood collar
    p.setBrush(QColor(228, 192, 142));
    QPainterPath collar;
    collar.moveTo(7.6, 13.6); collar.lineTo(5.6, 11.6); collar.lineTo(4.2, 13.0); collar.lineTo(6.2, 15.0);
    collar.closeSubpath();
    p.drawPath(collar);
    // Graphite tip
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(56, 56, 62));
    QPainterPath tip;
    tip.moveTo(6.2, 15.0); tip.lineTo(4.2, 13.0); tip.lineTo(1.8, 18.4); tip.closeSubpath();
    p.drawPath(tip);
    p.end();
    return pm;
}

// paint.net's eraser: a violet/magenta angled block with a lit top face.
QPixmap drawEraser() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(QColor(88, 42, 100), 1.0));
    // Top face (light magenta)
    p.setBrush(QColor(212, 130, 220));
    QPainterPath top;
    top.moveTo(12.8, 2.4); top.lineTo(18.0, 7.6); top.lineTo(10.4, 13.4); top.lineTo(5.2, 8.2);
    top.closeSubpath();
    p.drawPath(top);
    // Front face (deeper violet) -> gives the 3D block feel
    p.setBrush(QColor(150, 72, 168));
    QPainterPath front;
    front.moveTo(5.2, 8.2); front.lineTo(10.4, 13.4); front.lineTo(10.4, 17.2); front.lineTo(5.2, 12.0);
    front.closeSubpath();
    p.drawPath(front);
    // Side face (mid tone)
    p.setBrush(QColor(180, 100, 196));
    QPainterPath side;
    side.moveTo(10.4, 13.4); side.lineTo(18.0, 7.6); side.lineTo(18.0, 11.0); side.lineTo(10.4, 17.2);
    side.closeSubpath();
    p.drawPath(side);
    p.end();
    return pm;
}

// paint.net's paint bucket: a blue bucket tipped to the left, pouring paint.
QPixmap drawFill() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);

    p.save();
    p.translate(10.0, 9.5);
    p.rotate(-38);                 // tip the bucket like paint.net's
    p.translate(-10.0, -9.5);

    // Handle behind the body
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(76, 96, 122), 1.2));
    p.drawArc(QRectF(6.0, 1.6, 8.0, 7.0), 15 * 16, 150 * 16);

    // Tapered body with a blue sheen
    QLinearGradient bg(5.5, 0, 14.5, 0);
    bg.setColorAt(0.0, QColor(112, 170, 234));
    bg.setColorAt(1.0, QColor(40, 96, 172));
    p.setPen(QPen(QColor(26, 58, 104), 1.0));
    p.setBrush(bg);
    QPainterPath body;
    body.moveTo(5.2, 6.2); body.lineTo(14.8, 6.2);
    body.lineTo(12.9, 16.0); body.lineTo(7.1, 16.0);
    body.closeSubpath();
    p.drawPath(body);

    // Rim (lighter ellipse on top)
    p.setBrush(QColor(158, 206, 246));
    p.drawEllipse(QRectF(5.2, 4.1, 9.6, 4.2));
    p.restore();

    // Paint pouring out to the bottom-right
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(48, 112, 198));
    QPainterPath drip;
    drip.moveTo(12.6, 11.4);
    drip.quadTo(17.6, 14.0, 15.8, 18.4);
    drip.quadTo(13.4, 19.6, 12.2, 15.6);
    drip.closeSubpath();
    p.drawPath(drip);
    p.end();
    return pm;
}

// paint.net's colour picker: a blue-bulbed eyedropper.
QPixmap drawColorPicker() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Bulb
    p.setPen(QPen(QColor(28, 60, 102), 0.9));
    p.setBrush(QColor(92, 150, 216));
    p.drawEllipse(QRectF(10.8, 1.0, 7.8, 7.8));
    // Barrel
    p.setPen(QPen(QColor(92, 98, 110), 0.9));
    p.setBrush(QColor(202, 210, 222));
    QPainterPath shaft;
    shaft.moveTo(12.4, 7.4); shaft.lineTo(14.8, 9.8); shaft.lineTo(7.4, 15.6); shaft.lineTo(5.2, 13.4);
    shaft.closeSubpath();
    p.drawPath(shaft);
    // Tip
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(50, 54, 62));
    QPainterPath tip;
    tip.moveTo(7.4, 15.6); tip.lineTo(5.2, 13.4); tip.lineTo(1.8, 18.6); tip.closeSubpath();
    p.drawPath(tip);
    p.end();
    return pm;
}

// paint.net's recolor: a blue disc swept by a red "swap colour" arrow.
QPixmap drawRecolor() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Blue disc
    p.setPen(QPen(QColor(26, 58, 100), 0.9));
    p.setBrush(QColor(72, 128, 202));
    p.drawEllipse(QRectF(2.6, 5.4, 11.2, 11.2));
    // Red sweep
    QPen ap(QColor(212, 64, 42), 2.1);
    ap.setCapStyle(Qt::RoundCap);
    p.setPen(ap);
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(4.6, 2.4, 13.0, 13.0), 15 * 16, 205 * 16);
    // Arrow head
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(212, 64, 42));
    QPointF ah[] = { QPointF(18.6, 7.6), QPointF(13.6, 6.9), QPointF(16.3, 11.4) };
    p.drawPolygon(ah, 3);
    p.end();
    return pm;
}

// paint.net's clone stamp: dark knob + stem over a wide amber base.
QPixmap drawCloneStamp() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Knob
    p.setPen(QPen(QColor(38, 42, 50), 0.9));
    p.setBrush(QColor(82, 90, 104));
    p.drawEllipse(QRectF(7.0, 1.0, 6.0, 5.2));
    // Stem
    p.setBrush(QColor(112, 120, 134));
    p.drawRect(QRectF(8.6, 4.8, 2.8, 5.4));
    // Amber base
    p.setPen(QPen(QColor(140, 84, 20), 0.9));
    QLinearGradient bg(4, 10, 16, 15);
    bg.setColorAt(0.0, QColor(248, 182, 88));
    bg.setColorAt(1.0, QColor(210, 130, 36));
    p.setBrush(bg);
    QPainterPath base;
    base.moveTo(5.0, 10.2); base.lineTo(15.0, 10.2);
    base.lineTo(16.6, 14.2); base.lineTo(3.4, 14.2);
    base.closeSubpath();
    p.drawPath(base);
    // Foot
    p.setBrush(QColor(184, 110, 28));
    p.drawRect(QRectF(3.2, 14.6, 13.6, 2.6));
    p.end();
    return pm;
}

// paint.net's text tool: a single serif "T". Drawn in a mid slate tone so it reads
// on a light AND a dark palette (paint.net can use black — its palette is light).
QPixmap drawText() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QFont font;
    font.setPixelSize(18);
    font.setBold(true);
    font.setFamily("Serif");
    p.setFont(font);
    p.setPen(QColor(146, 162, 186));
    p.drawText(QRect(0, 0, TS, TS), Qt::AlignCenter, "T");
    p.end();
    return pm;
}

// paint.net's line/curve: an S-curve with its two control nodes.
QPixmap drawLine() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPen cp(QColor(158, 172, 192), 1.9);
    cp.setCapStyle(Qt::RoundCap);
    p.setPen(cp);
    p.setBrush(Qt::NoBrush);
    QPainterPath c;
    c.moveTo(3.4, 16.4);
    c.cubicTo(7.0, 6.0, 13.0, 18.0, 16.8, 4.0);
    p.drawPath(c);
    // Control nodes
    p.setPen(QPen(QColor(28, 60, 100), 0.9));
    p.setBrush(QColor(100, 154, 216));
    p.drawEllipse(QPointF(3.4, 16.4), 2.3, 2.3);
    p.drawEllipse(QPointF(16.8, 4.0), 2.3, 2.3);
    p.end();
    return pm;
}

// paint.net's shapes: a blue square, a green triangle and a purple circle overlapping.
QPixmap drawShape() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Blue square (back)
    p.setPen(QPen(QColor(28, 60, 104), 0.9));
    p.setBrush(QColor(80, 138, 210));
    p.drawRect(QRectF(1.6, 2.8, 9.4, 9.4));
    // Green triangle (right)
    p.setPen(QPen(QColor(34, 92, 52), 0.9));
    p.setBrush(QColor(106, 184, 110));
    QPointF tri[] = { QPointF(14.6, 6.2), QPointF(19.0, 16.8), QPointF(10.2, 16.8) };
    p.drawPolygon(tri, 3);
    // Purple circle (front)
    p.setPen(QPen(QColor(84, 40, 110), 0.9));
    p.setBrush(QColor(170, 98, 210));
    p.drawEllipse(QRectF(5.8, 8.2, 8.8, 8.8));
    p.end();
    return pm;
}

// paint.net's gradient icon: a violet -> blue ramp in a rounded square.
QPixmap drawGradient() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient grad(3, 3, 17, 17);
    grad.setColorAt(0.0, QColor(166, 100, 220));
    grad.setColorAt(1.0, QColor(56, 104, 198));
    p.setPen(QPen(QColor(44, 54, 74), 1.1));
    p.setBrush(grad);
    p.drawRoundedRect(QRectF(3, 3, 14, 14), 1.6, 1.6);
    p.end();
    return pm;
}

// paint.net's lasso: a rope loop with a tail — NOT a dashed circle (which read
// almost identically to the ellipse-select icon right below it).
QPixmap drawLasso() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPen rope(QColor(50, 106, 176), 1.7);
    rope.setCapStyle(Qt::RoundCap);
    p.setPen(rope);
    p.setBrush(QColor(178, 215, 245));
    p.drawEllipse(QRectF(3.2, 2.0, 12.4, 9.8));
    // Tail curling away from the loop
    p.setBrush(Qt::NoBrush);
    QPainterPath tail;
    tail.moveTo(9.0, 11.8);
    tail.cubicTo(9.4, 15.0, 13.8, 15.4, 12.2, 18.6);
    p.drawPath(tail);
    p.end();
    return pm;
}

// Move Selection: same silhouette as Move Selected Pixels but with a HOLLOW
// arrow — that outline/filled pair is exactly how paint.net tells them apart.
QPixmap drawMoveSelection() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPainterPath a;
    a.moveTo(2, 1); a.lineTo(2, 12.5); a.lineTo(5.0, 9.8);
    a.lineTo(7.0, 14.2); a.lineTo(8.9, 13.3); a.lineTo(7.0, 9.2);
    a.lineTo(11.0, 8.8); a.closeSubpath();
    QPen ap(QColor(104, 154, 212), 1.3);
    ap.setJoinStyle(Qt::RoundJoin);
    p.setPen(ap);
    p.setBrush(Qt::NoBrush);          // hollow
    p.drawPath(a);
    // Same compact move cross as the Move tool.
    const qreal cx = 14.5, cy = 14.5, r = 5.0;
    QPen cp(QColor(64, 84, 110), 1.3); cp.setCapStyle(Qt::RoundCap);
    p.setPen(cp);
    p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));
    p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(64, 84, 110));
    const qreal h = 2.0;
    QPointF up[] = {QPointF(cx, cy - r - 0.8), QPointF(cx - h, cy - r + 1.6), QPointF(cx + h, cy - r + 1.6)};
    QPointF dn[] = {QPointF(cx, cy + r + 0.8), QPointF(cx - h, cy + r - 1.6), QPointF(cx + h, cy + r - 1.6)};
    QPointF lf[] = {QPointF(cx - r - 0.8, cy), QPointF(cx - r + 1.6, cy - h), QPointF(cx - r + 1.6, cy + h)};
    QPointF rt[] = {QPointF(cx + r + 0.8, cy), QPointF(cx + r - 1.6, cy - h), QPointF(cx + r - 1.6, cy + h)};
    p.drawPolygon(up, 3); p.drawPolygon(dn, 3); p.drawPolygon(lf, 3); p.drawPolygon(rt, 3);
    p.end();
    return pm;
}

// paint.net's zoom: a magnifier with a glassy blue lens.
QPixmap drawZoom() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Handle (drawn first, behind the lens)
    QPen hp(QColor(78, 88, 104), 2.7);
    hp.setCapStyle(Qt::RoundCap);
    p.setPen(hp);
    p.drawLine(QPointF(12.4, 12.4), QPointF(17.6, 17.6));
    // Lens
    QRadialGradient lg(7.2, 6.6, 7.0);
    lg.setColorAt(0.0, QColor(212, 236, 252));
    lg.setColorAt(1.0, QColor(118, 176, 226));
    p.setPen(QPen(QColor(36, 72, 118), 1.6));
    p.setBrush(lg);
    p.drawEllipse(QRectF(2.2, 2.2, 11.6, 11.6));
    // Plus
    QPen pp(QColor(36, 72, 118), 1.5);
    pp.setCapStyle(Qt::RoundCap);
    p.setPen(pp);
    p.drawLine(QPointF(8.0, 4.9), QPointF(8.0, 11.1));
    p.drawLine(QPointF(4.9, 8.0), QPointF(11.1, 8.0));
    p.end();
    return pm;
}

QPixmap drawPan() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Clean open "grab" hand, in paint.net's warm skin tone. The outline is a warm
    // brown (not neutral ink) so the icon reads on a light AND a dark palette
    // without needing themed().
    const QColor ink(150, 98, 54);
    const QColor skin(243, 208, 168);
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

// Every tool icon is now drawn in real paint.net-like colours, so none of them go
// through themed() — that lightening pass exists only to rescue dark line art and
// would wash these out.
QIcon forTool(ToolType type) {
    switch (type) {
    case ToolType::RectSelection:   return QIcon(drawRectSelect());
    case ToolType::EllipseSelection:return QIcon(drawEllipseSelect());
    case ToolType::LassoSelection:  return QIcon(drawLasso());
    case ToolType::MagicWand:       return QIcon(drawMagicWand());
    case ToolType::Move:            return QIcon(drawMove());
    case ToolType::MoveSelection:   return QIcon(drawMoveSelection());
    case ToolType::Zoom:            return QIcon(drawZoom());
    case ToolType::Pan:             return QIcon(drawPan());
    case ToolType::Fill:            return QIcon(drawFill());
    case ToolType::Gradient:        return QIcon(drawGradient());
    case ToolType::Brush:           return QIcon(drawBrush());
    case ToolType::Eraser:          return QIcon(drawEraser());
    case ToolType::Pencil:          return QIcon(drawPencil());
    case ToolType::ColorPicker:     return QIcon(drawColorPicker());
    case ToolType::CloneStamp:      return QIcon(drawCloneStamp());
    case ToolType::Recolor:         return QIcon(drawRecolor());
    case ToolType::Text:            return QIcon(drawText());
    case ToolType::Line:            return QIcon(drawLine());
    case ToolType::Shape:           return QIcon(drawShape());
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
