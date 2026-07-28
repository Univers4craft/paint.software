#include "shapetool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include "core/hatchpatterns.h"
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <cmath>

namespace {
constexpr double kGrabRadius = 8.0;    // widget-space pixels to grab a nub
constexpr double kRotOffset  = 24.0;   // widget-space gap from the top edge to the rotation handle
constexpr double kMinSize    = 1.0;    // never let a shape collapse to nothing
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

void ShapeTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer || layer->isLocked()) return;

    if (m_state == State::Editing) {
        // Clicking a nub grabs it; clicking inside moves the shape; clicking away
        // commits this shape and immediately starts a new one from here.
        const Handle h = handleAt(event->position(), canvas);
        if (h != Handle::None) {
            m_activeHandle = h;
            m_pressCanvas = canvasPos;
            m_rectAtPress = m_rect;
            return;
        }
        commit(canvas);
    }

    m_state = State::Dragging;
    m_activeHandle = Handle::None;
    m_angle = 0.0;
    m_startPos = canvasPos;
    m_currentPos = canvasPos;
    m_modifiers = event->modifiers();
    m_beforeImage = layer->image().copy();
    m_useRight = (event->button() == Qt::RightButton);
    canvas.update();
}

void ShapeTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (m_state == State::Dragging) {
        m_currentPos = canvasPos;
        m_modifiers = event->modifiers();
        canvas.update();
        return;
    }
    if (m_state != State::Editing || m_activeHandle == Handle::None) return;

    if (m_activeHandle == Handle::Move) {
        m_rect = m_rectAtPress.translated(canvasPos - m_pressCanvas);
    } else if (m_activeHandle == Handle::Rotate) {
        const QPointF c = m_rect.center();
        double deg = std::atan2(canvasPos.y() - c.y(), canvasPos.x() - c.x()) * 180.0 / M_PI + 90.0;
        if (event->modifiers() & Qt::ShiftModifier)
            deg = std::round(deg / 15.0) * 15.0;   // snap to 15°
        m_angle = deg;
    } else {
        resizeTo(m_activeHandle, canvasPos, event->modifiers());
    }
    canvas.update();
}

void ShapeTool::mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (m_state == State::Dragging) {
        const QRectF r = effectiveRect();
        // A zero-size drag (a plain click) is not a shape; drop back to Idle.
        if (r.width() < kMinSize || r.height() < kMinSize) { m_state = State::Idle; return; }
        m_rect = r;
        m_angle = 0.0;
        m_state = State::Editing;   // show the nubs for editing
        canvas.update();
    } else if (m_state == State::Editing) {
        Q_UNUSED(canvasPos);
        m_activeHandle = Handle::None;
    }
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

bool ShapeTool::wantsCommitKey(int key) const {
    if (m_state != State::Editing) return false;
    switch (key) {
    case Qt::Key_Return: case Qt::Key_Enter: case Qt::Key_Escape:
    case Qt::Key_Left: case Qt::Key_Right: case Qt::Key_Up: case Qt::Key_Down:
        return true;
    default:
        return false;
    }
}

void ShapeTool::keyPressEvent(QKeyEvent *event, CanvasWidget &canvas) {
    if (m_state != State::Editing) return;
    switch (event->key()) {
    case Qt::Key_Return: case Qt::Key_Enter:
        commit(canvas);
        event->accept();
        break;
    case Qt::Key_Escape:
        // Discard: the layer was never modified during editing.
        m_state = State::Idle;
        m_activeHandle = Handle::None;
        canvas.update();
        event->accept();
        break;
    case Qt::Key_Left: case Qt::Key_Right: case Qt::Key_Up: case Qt::Key_Down: {
        const double step = (event->modifiers() & Qt::ControlModifier) ? 10.0 : 1.0;
        double dx = 0, dy = 0;
        if (event->key() == Qt::Key_Left)  dx = -step;
        if (event->key() == Qt::Key_Right) dx =  step;
        if (event->key() == Qt::Key_Up)    dy = -step;
        if (event->key() == Qt::Key_Down)  dy =  step;
        m_rect.translate(dx, dy);
        canvas.update();
        event->accept();
        break;
    }
    default:
        break;
    }
}

void ShapeTool::deactivate(CanvasWidget &canvas) {
    if (m_state == State::Editing) commit(canvas);
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

QRectF ShapeTool::effectiveRect() const {
    double dx = m_currentPos.x() - m_startPos.x();
    double dy = m_currentPos.y() - m_startPos.y();

    if (m_modifiers & Qt::ShiftModifier) {
        const double side = qMax(std::abs(dx), std::abs(dy));
        dx = (dx < 0) ? -side : side;
        dy = (dy < 0) ? -side : side;
    }
    if (m_modifiers & Qt::AltModifier)
        return QRectF(m_startPos.x() - dx, m_startPos.y() - dy, 2 * dx, 2 * dy).normalized();
    return QRectF(m_startPos.x(), m_startPos.y(), dx, dy).normalized();
}

QTransform ShapeTool::rotationTransform(const QRectF &rect, double angleDeg) {
    const QPointF c = rect.center();
    QTransform t;
    t.translate(c.x(), c.y());
    t.rotate(angleDeg);
    t.translate(-c.x(), -c.y());
    return t;
}

QPointF ShapeTool::handleLocal(Handle h, const QRectF &r) {
    const double cx = r.center().x(), cy = r.center().y();
    switch (h) {
    case Handle::TopLeft:     return r.topLeft();
    case Handle::Top:         return QPointF(cx, r.top());
    case Handle::TopRight:    return r.topRight();
    case Handle::Right:       return QPointF(r.right(), cy);
    case Handle::BottomRight: return r.bottomRight();
    case Handle::Bottom:      return QPointF(cx, r.bottom());
    case Handle::BottomLeft:  return r.bottomLeft();
    case Handle::Left:        return QPointF(r.left(), cy);
    default:                  return r.center();
    }
}

QPointF ShapeTool::handleWidget(Handle h, const CanvasWidget &canvas) const {
    const QTransform rot = rotationTransform(m_rect, m_angle);
    if (h == Handle::Rotate) {
        // The rotation handle floats a fixed screen distance above the top edge.
        const QPointF topMidWidget = canvas.canvasToWidget(rot.map(QPointF(m_rect.center().x(), m_rect.top())));
        const QPointF centreWidget = canvas.canvasToWidget(rot.map(m_rect.center()));
        QPointF dir = topMidWidget - centreWidget;
        const double len = std::hypot(dir.x(), dir.y());
        if (len > 1e-6) dir /= len; else dir = QPointF(0, -1);
        return topMidWidget + dir * kRotOffset;
    }
    return canvas.canvasToWidget(rot.map(handleLocal(h, m_rect)));
}

ShapeTool::Handle ShapeTool::handleAt(const QPointF &widgetPos, const CanvasWidget &canvas) const {
    const Handle order[] = { Handle::Rotate, Handle::TopLeft, Handle::TopRight,
                             Handle::BottomRight, Handle::BottomLeft, Handle::Top,
                             Handle::Right, Handle::Bottom, Handle::Left };
    for (Handle h : order) {
        const QPointF w = handleWidget(h, canvas);
        if (QLineF(widgetPos, w).length() <= kGrabRadius) return h;
    }
    // Inside the (rotated) box → move.
    const QPointF canv = canvas.widgetToCanvas(widgetPos);
    const QPointF unrot = rotationTransform(m_rect, m_angle).inverted().map(canv);
    if (m_rect.contains(unrot)) return Handle::Move;
    return Handle::None;
}

void ShapeTool::resizeTo(Handle h, const QPointF &canvasPos, Qt::KeyboardModifiers mods) {
    // Sign of the handle along each axis: which edges it moves.
    int sx = 0, sy = 0;
    switch (h) {
    case Handle::TopLeft:     sx = -1; sy = -1; break;
    case Handle::Top:         sx =  0; sy = -1; break;
    case Handle::TopRight:    sx =  1; sy = -1; break;
    case Handle::Right:       sx =  1; sy =  0; break;
    case Handle::BottomRight: sx =  1; sy =  1; break;
    case Handle::Bottom:      sx =  0; sy =  1; break;
    case Handle::BottomLeft:  sx = -1; sy =  1; break;
    case Handle::Left:        sx = -1; sy =  0; break;
    default: return;
    }

    const QRectF r = m_rect;
    const double oldW = r.width(), oldH = r.height();
    const QPointF centre = r.center();

    QTransform rot;   rot.rotate(m_angle);           // pure rotation (no translation)
    const QTransform unrot = rot.inverted();

    double newW = oldW, newH = oldH;
    QPointF newCentre = centre;

    if (mods & Qt::AltModifier) {
        // Resize about the centre: half-extents follow the mouse.
        const QPointF local = unrot.map(canvasPos - centre);
        if (sx) newW = 2 * std::abs(local.x());
        if (sy) newH = 2 * std::abs(local.y());
    } else {
        // Anchor the opposite handle in world space; the dragged handle follows.
        const QPointF anchorLocalVec(-sx * oldW / 2.0, -sy * oldH / 2.0);
        const QPointF anchorWorld = centre + rot.map(anchorLocalVec);
        const QPointF local = unrot.map(canvasPos - anchorWorld);
        if (sx) newW = std::abs(local.x());
        if (sy) newH = std::abs(local.y());

        if ((mods & Qt::ShiftModifier) && sx && sy) {   // keep aspect (corners only)
            const double f = qMax(newW / qMax(oldW, kMinSize), newH / qMax(oldH, kMinSize));
            newW = oldW * f;
            newH = oldH * f;
        }
        newW = qMax(newW, kMinSize);
        newH = qMax(newH, kMinSize);
        newCentre = anchorWorld - rot.map(QPointF(-sx * newW / 2.0, -sy * newH / 2.0));
    }

    newW = qMax(newW, kMinSize);
    newH = qMax(newH, kMinSize);
    m_rect = QRectF(newCentre.x() - newW / 2.0, newCentre.y() - newH / 2.0, newW, newH);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void ShapeTool::paintShape(QPainter &painter, const QColor &primary, const QColor &secondary,
                           const QRectF &rect) const {
    QPen pen(primary, m_brushSize);
    painter.setPen((m_shapeFill == ShapeFill::Filled) ? Qt::NoPen : pen);
    const QBrush fill = (m_fillStyle > 0) ? Hatch::brush(m_fillStyle, primary, secondary)
                                          : QBrush(secondary);
    painter.setBrush((m_shapeFill == ShapeFill::Outline) ? Qt::NoBrush : fill);
    painter.drawPath(shapePath(rect));
}

void ShapeTool::commit(CanvasWidget &canvas) {
    auto *doc = canvas.document();
    auto *layer = doc ? doc->activeLayer() : nullptr;
    if (!layer || m_beforeImage.isNull()) { m_state = State::Idle; return; }

    layer->setImage(m_beforeImage.copy());
    QPainter painter(&layer->image());
    if (m_antialiased) painter.setRenderHint(QPainter::Antialiasing);
    clipToSelection(painter, doc);
    painter.setOpacity(m_opacity / 100.0);
    painter.setWorldTransform(rotationTransform(m_rect, m_angle));

    const QColor primary = m_useRight ? doc->secondaryColor() : doc->primaryColor();
    const QColor secondary = m_useRight ? doc->primaryColor() : doc->secondaryColor();
    paintShape(painter, primary, secondary, m_rect);
    painter.end();

    doc->pushImageEdit(doc->activeLayerIndex(), m_beforeImage, "Shape");
    m_state = State::Idle;
    m_activeHandle = Handle::None;
    canvas.update();
}

void ShapeTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    if (m_state == State::Idle) return;

    const QRectF rect = (m_state == State::Dragging) ? effectiveRect() : m_rect;
    const double angle = (m_state == State::Dragging) ? 0.0 : m_angle;
    if (rect.width() < kMinSize || rect.height() < kMinSize) return;

    auto *doc = canvas.document();
    const QColor primary = m_useRight ? doc->secondaryColor() : doc->primaryColor();
    const QColor secondary = m_useRight ? doc->primaryColor() : doc->secondaryColor();

    // Build the canvas→widget affine, then compose rotation-about-centre on top, so
    // a path built in canvas coords renders rotated and zoomed in widget space —
    // and the pen width scales with the zoom, matching the committed raster.
    const double z = canvas.zoom();
    const QPointF b = canvas.canvasToWidget(QPointF(0, 0));
    const QTransform toWidget(z, 0, 0, z, b.x(), b.y());
    const QTransform full = rotationTransform(rect, angle) * toWidget;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(m_opacity / 100.0);
    painter.setTransform(full);
    paintShape(painter, primary, secondary, rect);
    painter.restore();

    if (m_state != State::Editing) return;

    // The bounding box, the eight resize nubs, and the rotation handle — all drawn
    // in plain widget space so they keep a constant on-screen size.
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(1.0);

    const QColor accent(0, 120, 215);
    painter.setPen(QPen(accent, 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    QPolygonF boxPoly;
    for (Handle h : { Handle::TopLeft, Handle::TopRight, Handle::BottomRight, Handle::BottomLeft })
        boxPoly << handleWidget(h, canvas);
    painter.drawPolygon(boxPoly);

    // Connector from the top edge to the rotation handle.
    const QPointF topMid = handleWidget(Handle::Top, canvas);
    const QPointF rotPos = handleWidget(Handle::Rotate, canvas);
    painter.drawLine(topMid, rotPos);

    painter.setPen(QPen(Qt::black, 1));
    for (Handle h : { Handle::TopLeft, Handle::Top, Handle::TopRight, Handle::Right,
                      Handle::BottomRight, Handle::Bottom, Handle::BottomLeft, Handle::Left }) {
        painter.setBrush(Qt::white);
        const QPointF w = handleWidget(h, canvas);
        painter.drawRect(QRectF(w.x() - 4, w.y() - 4, 8, 8));
    }
    painter.setBrush(accent);
    painter.drawEllipse(rotPos, 4, 4);
    painter.restore();
}

// ---------------------------------------------------------------------------
// Shape geometry — generic forms built from the bounding rect.
// ---------------------------------------------------------------------------

namespace {
// Regular N-gon inscribed in the rect, first vertex pointing up.
QPainterPath regularPolygon(const QRectF &rect, int n) {
    QPainterPath path;
    const double cx = rect.center().x(), cy = rect.center().y();
    const double rx = rect.width() / 2, ry = rect.height() / 2;
    for (int i = 0; i < n; ++i) {
        const double a = -M_PI / 2 + i * 2 * M_PI / n;
        const QPointF p(cx + rx * std::cos(a), cy + ry * std::sin(a));
        if (i == 0) path.moveTo(p); else path.lineTo(p);
    }
    path.closeSubpath();
    return path;
}

// N-point star; outer vertices on the rect ellipse, inner at innerRatio.
QPainterPath starPolygon(const QRectF &rect, int points, double innerRatio) {
    QPainterPath path;
    const double cx = rect.center().x(), cy = rect.center().y();
    const double rx = rect.width() / 2, ry = rect.height() / 2;
    const int n = points * 2;
    for (int i = 0; i < n; ++i) {
        const double a = -M_PI / 2 + i * M_PI / points;
        const double r = (i % 2 == 0) ? 1.0 : innerRatio;
        const QPointF p(cx + rx * r * std::cos(a), cy + ry * r * std::sin(a));
        if (i == 0) path.moveTo(p); else path.lineTo(p);
    }
    path.closeSubpath();
    return path;
}
} // namespace

QPainterPath ShapeTool::shapePath(const QRectF &rect) const {
    const double L = rect.left(), T = rect.top(), R = rect.right(), B = rect.bottom();
    const double W = rect.width(), H = rect.height();
    const double cx = rect.center().x(), cy = rect.center().y();
    auto f = [&](double fx, double fy) { return QPointF(L + W * fx, T + H * fy); };

    QPainterPath path;
    switch (m_shapeType) {
    case ShapeType::Rectangle:
        path.addRect(rect);
        break;
    case ShapeType::RoundedRectangle:
        path.addRoundedRect(rect, m_cornerSize, m_cornerSize);
        break;
    case ShapeType::Ellipse:
        path.addEllipse(rect);
        break;
    case ShapeType::Diamond:
        path.moveTo(cx, T); path.lineTo(R, cy); path.lineTo(cx, B); path.lineTo(L, cy);
        path.closeSubpath();
        break;
    case ShapeType::Triangle:
        path.moveTo(cx, T); path.lineTo(R, B); path.lineTo(L, B);
        path.closeSubpath();
        break;
    case ShapeType::Trapezoid:
        path.moveTo(f(0.25, 0)); path.lineTo(f(0.75, 0));
        path.lineTo(R, B); path.lineTo(L, B);
        path.closeSubpath();
        break;
    case ShapeType::Parallelogram:
        path.moveTo(f(0.25, 0)); path.lineTo(R, T);
        path.lineTo(f(0.75, 1)); path.lineTo(L, B);
        path.closeSubpath();
        break;
    case ShapeType::RightTriangle:
        path.moveTo(L, T); path.lineTo(L, B); path.lineTo(R, B);
        path.closeSubpath();
        break;
    case ShapeType::Pentagon: return regularPolygon(rect, 5);
    case ShapeType::Hexagon:  return regularPolygon(rect, 6);
    case ShapeType::Heptagon: return regularPolygon(rect, 7);
    case ShapeType::Octagon:  return regularPolygon(rect, 8);
    case ShapeType::Star3: return starPolygon(rect, 3, 0.42);
    case ShapeType::Star4: return starPolygon(rect, 4, 0.40);
    case ShapeType::Star:  return starPolygon(rect, 5, 0.40);
    case ShapeType::Star6: return starPolygon(rect, 6, 0.50);
    case ShapeType::Arrow: {
        // Block arrow pointing right.
        path.moveTo(f(0.0, 0.30)); path.lineTo(f(0.6, 0.30)); path.lineTo(f(0.6, 0.05));
        path.lineTo(f(1.0, 0.50)); path.lineTo(f(0.6, 0.95)); path.lineTo(f(0.6, 0.70));
        path.lineTo(f(0.0, 0.70));
        path.closeSubpath();
        break;
    }
    case ShapeType::Chevron: {
        // Right-pointing chevron with a notch on the trailing edge.
        path.moveTo(f(0.0, 0.0)); path.lineTo(f(0.55, 0.0)); path.lineTo(f(1.0, 0.5));
        path.lineTo(f(0.55, 1.0)); path.lineTo(f(0.0, 1.0)); path.lineTo(f(0.45, 0.5));
        path.closeSubpath();
        break;
    }
    case ShapeType::SpeechBalloon: {
        QRectF body(L, T, W, H * 0.72);
        const double rr = qMin(W, H) * 0.18;
        QPainterPath b;
        b.addRoundedRect(body, rr, rr);
        QPainterPath tail;
        tail.moveTo(f(0.25, 0.70)); tail.lineTo(f(0.20, 1.0)); tail.lineTo(f(0.45, 0.70));
        tail.closeSubpath();
        return b.united(tail);
    }
    case ShapeType::Cloud: {
        // Union of overlapping lobes — a generic puffy cloud.
        QPainterPath c;
        const struct { double fx, fy, fr; } lobes[] = {
            {0.28, 0.62, 0.22}, {0.50, 0.45, 0.28}, {0.70, 0.60, 0.24},
            {0.40, 0.68, 0.20}, {0.60, 0.68, 0.20},
        };
        for (const auto &lo : lobes) {
            const double rx = W * lo.fr, ry = H * lo.fr * 1.2;
            QPainterPath e;
            e.addEllipse(QPointF(L + W * lo.fx, T + H * lo.fy), rx, ry);
            c = c.united(e);
        }
        c.addRect(QRectF(f(0.28, 0.62), f(0.72, 0.85)));
        return c.simplified();
    }
    case ShapeType::Heart: {
        path.moveTo(f(0.5, 0.30));
        path.cubicTo(f(0.42, -0.06), f(-0.05, 0.18), f(0.03, 0.42));
        path.cubicTo(f(0.10, 0.62), f(0.32, 0.78), f(0.5, 0.97));
        path.cubicTo(f(0.68, 0.78), f(0.90, 0.62), f(0.97, 0.42));
        path.cubicTo(f(1.05, 0.18), f(0.58, -0.06), f(0.5, 0.30));
        path.closeSubpath();
        break;
    }
    case ShapeType::Lightning: {
        path.moveTo(f(0.52, 0.0)); path.lineTo(f(0.18, 0.55)); path.lineTo(f(0.44, 0.55));
        path.lineTo(f(0.34, 1.0)); path.lineTo(f(0.82, 0.42)); path.lineTo(f(0.54, 0.42));
        path.lineTo(f(0.70, 0.0));
        path.closeSubpath();
        break;
    }
    case ShapeType::Cross: {
        const double t = 0.34;   // arm thickness as a fraction
        const double x1 = 0.5 - t / 2, x2 = 0.5 + t / 2;
        const double y1 = 0.5 - t / 2, y2 = 0.5 + t / 2;
        path.moveTo(f(x1, 0));  path.lineTo(f(x2, 0));  path.lineTo(f(x2, y1));
        path.lineTo(f(1, y1));  path.lineTo(f(1, y2));  path.lineTo(f(x2, y2));
        path.lineTo(f(x2, 1));  path.lineTo(f(x1, 1));  path.lineTo(f(x1, y2));
        path.lineTo(f(0, y2));  path.lineTo(f(0, y1));  path.lineTo(f(x1, y1));
        path.closeSubpath();
        break;
    }
    case ShapeType::Check: {
        path.moveTo(f(0.05, 0.50)); path.lineTo(f(0.20, 0.38)); path.lineTo(f(0.40, 0.60));
        path.lineTo(f(0.82, 0.10)); path.lineTo(f(0.95, 0.24)); path.lineTo(f(0.40, 0.92));
        path.closeSubpath();
        break;
    }
    }
    return path;
}
