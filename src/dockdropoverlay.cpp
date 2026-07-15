#include "dockdropoverlay.h"

#include <QDockWidget>
#include <QPainter>
#include <QPainterPath>

// ── Construction ────────────────────────────────────────────────────────────

DockDropOverlay::DockDropOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    hide();
}

// ── Public interface ─────────────────────────────────────────────────────────

void DockDropOverlay::updateTarget(QDockWidget *target, Zone hoveredZone)
{
    if (!target) { hideOverlay(); return; }

    m_targetDock = target;
    m_zone       = hoveredZone;

    // Cover the docked panel; if we have a parent, use parent-local coords.
    QPoint tl;
    if (parentWidget()) {
        tl = target->mapTo(parentWidget(), QPoint(0, 0));
    } else {
        tl = target->mapToGlobal(QPoint(0, 0));
    }
    setGeometry(QRect(tl, target->size()));

    if (!isVisible()) show();
    raise();
    update();
}

void DockDropOverlay::hideOverlay()
{
    hide();
    m_targetDock = nullptr;
    m_zone       = None;
}

// ── Zone geometry ────────────────────────────────────────────────────────────

QRect DockDropOverlay::zoneRect(Zone zone) const
{
    return zoneRectForSize(zone, size());
}

QRect DockDropOverlay::zoneRectForSize(Zone zone, const QSize &sz)
{
    const int w = sz.width();
    const int h = sz.height();

    auto clamp = [](int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); };
    const int sx = clamp(w * 30 / 100, 32, 80);
    const int sy = clamp(h * 30 / 100, 32, 80);

    switch (zone) {
    case Top:    return QRect(0,    0,    w,  sy);
    case Bottom: return QRect(0,    h-sy, w,  sy);
    case Left:   return QRect(0,    0,    sx, h);
    case Right:  return QRect(w-sx, 0,    sx, h);
    case Center: return QRect(sx,   sy,   w-2*sx, h-2*sy);
    default:     return {};
    }
}

// ── Painting ─────────────────────────────────────────────────────────────────

static void drawFilledArrow(QPainter &p, const QRect &r, DockDropOverlay::Zone zone)
{
    const int cx = r.left() + r.width()  / 2;
    const int cy = r.top()  + r.height() / 2;
    const int sz = qMin(r.width(), r.height()) / 3;

    QPainterPath path;
    switch (zone) {
    case DockDropOverlay::Top:
        path.moveTo(cx,      cy - sz);
        path.lineTo(cx + sz, cy + sz / 2);
        path.lineTo(cx - sz, cy + sz / 2);
        break;
    case DockDropOverlay::Bottom:
        path.moveTo(cx,      cy + sz);
        path.lineTo(cx + sz, cy - sz / 2);
        path.lineTo(cx - sz, cy - sz / 2);
        break;
    case DockDropOverlay::Left:
        path.moveTo(cx - sz,     cy);
        path.lineTo(cx + sz / 2, cy - sz);
        path.lineTo(cx + sz / 2, cy + sz);
        break;
    case DockDropOverlay::Right:
        path.moveTo(cx + sz,     cy);
        path.lineTo(cx - sz / 2, cy - sz);
        path.lineTo(cx - sz / 2, cy + sz);
        break;
    case DockDropOverlay::Center: {
        // Draw a small "tab stack" icon: two overlapping rectangles.
        const int hw = sz * 3 / 4, hh = sz / 2;
        p.drawRect(QRect(cx - hw + 4, cy - hh + 4, hw * 2, hh * 2));
        p.drawRect(QRect(cx - hw,     cy - hh,     hw * 2, hh * 2));
        return;
    }
    default:
        return;
    }
    path.closeSubpath();
    p.fillPath(path, p.pen().color());
}

void DockDropOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Subtle full-coverage tint so the user knows dragging is active.
    p.fillRect(rect(), QColor(0, 120, 215, 18));

    const QColor inactiveFill(0, 120, 215, 45);
    const QColor activeFill  (0, 120, 215, 140);
    const QColor inactiveBorder(80, 160, 255, 130);
    const QColor activeBorder (255, 255, 255, 230);

    for (Zone z : {Top, Bottom, Left, Right, Center}) {
        const QRect r = zoneRect(z);
        if (r.isEmpty()) continue;

        const bool active = (z == m_zone);

        // Fill
        p.setPen(Qt::NoPen);
        p.fillRect(r, active ? activeFill : inactiveFill);

        // Border
        p.setPen(QPen(active ? activeBorder : inactiveBorder, active ? 2 : 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(1, 1, -1, -1));

        // Arrow / icon
        if (active) {
            p.setPen(QPen(QColor(255, 255, 255, 230), 2));
            p.setBrush(QColor(255, 255, 255, 230));
            drawFilledArrow(p, r, z);
        }
    }
}
