#include "linetool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <cmath>

namespace {
constexpr double kGrabRadius = 8.0;   // widget-space pixels to grab a node
}

void LineTool::setStraight(const QPointF &a, const QPointF &b) {
    m_pts[0] = a;
    m_pts[1] = a + (b - a) / 3.0;
    m_pts[2] = a + (b - a) * 2.0 / 3.0;
    m_pts[3] = b;
}

int LineTool::nodeAt(const QPointF &widgetPos, const CanvasWidget &canvas) const {
    int best = -1;
    double bestD = kGrabRadius;
    for (int i = 0; i < 4; ++i) {
        const double d = QLineF(widgetPos, canvas.canvasToWidget(m_pts[i])).length();
        if (d <= bestD) { bestD = d; best = i; }
    }
    return best;
}

QPainterPath LineTool::buildPath(CurveType type, const std::array<QPointF, 4> &pts) {
    QPainterPath path(pts[0]);
    switch (type) {
    case CurveType::Straight:
        // A polyline running straight through all four nubs.
        path.lineTo(pts[1]);
        path.lineTo(pts[2]);
        path.lineTo(pts[3]);
        break;
    case CurveType::CubicSpline: {
        // A Catmull-Rom spline interpolating all four nubs, expressed as three
        // cubic Bézier segments. The tangent at each interior point follows the
        // line between its neighbours; the endpoints reuse their sole neighbour
        // (a natural, non-overshooting end condition). Standard conversion:
        // for a segment P1->P2 with neighbours P0 and P3, the Bézier controls are
        //   C1 = P1 + (P2 - P0) / 6,  C2 = P2 - (P3 - P1) / 6.
        const QPointF p[4] = {pts[0], pts[1], pts[2], pts[3]};
        for (int i = 0; i < 3; ++i) {
            const QPointF p0 = (i == 0) ? p[0] : p[i - 1];
            const QPointF p1 = p[i];
            const QPointF p2 = p[i + 1];
            const QPointF p3 = (i == 2) ? p[3] : p[i + 2];
            const QPointF c1 = p1 + (p2 - p0) / 6.0;
            const QPointF c2 = p2 - (p3 - p1) / 6.0;
            path.cubicTo(c1, c2, p2);
        }
        break;
    }
    case CurveType::Bezier:
        // End nubs are anchors, inner nubs are the cubic Bézier's handles.
        path.cubicTo(pts[1], pts[2], pts[3]);
        break;
    }
    return path;
}

QPainterPath LineTool::curvePath() const {
    return buildPath(m_curveType, m_pts);
}

void LineTool::paintCurve(QPainter &painter, const QColor &color, bool antialiased) const {
    if (antialiased) painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, m_brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (m_lineStyle == LineStyle::DashedLine) pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(curvePath());

    if (m_lineStyle == LineStyle::Arrow) {
        // Arrowhead aligned with the curve's tangent at the end point.
        const QPointF tip = m_pts[3];
        const QPointF from = (m_pts[2] != m_pts[3]) ? m_pts[2] : m_pts[0];
        const double angle = std::atan2(tip.y() - from.y(), tip.x() - from.x());
        const double s = m_brushSize * 3;
        const QPointF p1(tip.x() - s * std::cos(angle - M_PI / 6), tip.y() - s * std::sin(angle - M_PI / 6));
        const QPointF p2(tip.x() - s * std::cos(angle + M_PI / 6), tip.y() - s * std::sin(angle + M_PI / 6));
        QPainterPath arrow(tip);
        arrow.lineTo(p1);
        arrow.lineTo(p2);
        arrow.closeSubpath();
        painter.setBrush(color);
        painter.drawPath(arrow);
    }
}

void LineTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer || layer->isLocked()) return;

    if (m_state == State::Editing) {
        // Grab a node if the click landed on one; otherwise commit this curve and
        // begin a fresh line from here.
        const int node = nodeAt(event->position(), canvas);
        if (node >= 0) { m_activeNode = node; return; }
        commit(canvas);
    }

    m_state = State::Dragging;
    m_activeNode = -1;
    m_color = (event->button() == Qt::LeftButton) ? canvas.document()->primaryColor()
                                                  : canvas.document()->secondaryColor();
    m_beforeImage = layer->image().copy();
    setStraight(canvasPos, canvasPos);
    canvas.update();
}

void LineTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (m_state == State::Dragging) {
        setStraight(m_pts[0], canvasPos);
        canvas.update();
    } else if (m_state == State::Editing && m_activeNode >= 0) {
        m_pts[m_activeNode] = canvasPos;
        canvas.update();
    }
}

void LineTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &canvas) {
    if (m_state == State::Dragging) {
        // A zero-length drag (a plain click) is not a line; drop back to Idle.
        if (m_pts[0] == m_pts[3]) { m_state = State::Idle; return; }
        m_state = State::Editing;   // show the nubs for curve editing
        canvas.update();
    } else if (m_state == State::Editing) {
        m_activeNode = -1;
    }
}

bool LineTool::wantsCommitKey(int key) const {
    return m_state == State::Editing
        && (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Escape);
}

void LineTool::keyPressEvent(QKeyEvent *event, CanvasWidget &canvas) {
    if (m_state != State::Editing) return;
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        commit(canvas);
        event->accept();
    } else if (event->key() == Qt::Key_Escape) {
        // Discard: the layer was never modified during editing.
        m_state = State::Idle;
        m_activeNode = -1;
        canvas.update();
        event->accept();
    }
}

void LineTool::deactivate(CanvasWidget &canvas) {
    if (m_state == State::Editing) commit(canvas);
}

void LineTool::commit(CanvasWidget &canvas) {
    auto *doc = canvas.document();
    auto *layer = doc ? doc->activeLayer() : nullptr;
    if (!layer || m_beforeImage.isNull()) { m_state = State::Idle; return; }

    layer->setImage(m_beforeImage.copy());
    QPainter painter(&layer->image());
    clipToSelection(painter, doc);
    painter.setOpacity(m_opacity / 100.0);
    paintCurve(painter, m_color, m_antialiased);
    painter.end();

    doc->pushImageEdit(doc->activeLayerIndex(), m_beforeImage, "Line / Curve");
    m_state = State::Idle;
    m_activeNode = -1;
    canvas.update();
}

void LineTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    if (m_state == State::Idle) return;

    // Preview the curve itself in widget space, scaled to the zoom, so nothing is
    // baked into the layer until commit.
    painter.save();
    QPen pen(m_color, std::max(1.0, m_brushSize * canvas.zoom()), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (m_lineStyle == LineStyle::DashedLine) pen.setStyle(Qt::DashLine);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(m_opacity / 100.0);
    painter.setPen(pen);
    const std::array<QPointF, 4> widgetPts = {
        canvas.canvasToWidget(m_pts[0]), canvas.canvasToWidget(m_pts[1]),
        canvas.canvasToWidget(m_pts[2]), canvas.canvasToWidget(m_pts[3])};
    painter.drawPath(buildPath(m_curveType, widgetPts));
    painter.restore();

    if (m_state != State::Editing) return;

    // The four control nubs, plus thin handle lines linking each inner control
    // point to its endpoint.
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
    painter.drawLine(canvas.canvasToWidget(m_pts[0]), canvas.canvasToWidget(m_pts[1]));
    painter.drawLine(canvas.canvasToWidget(m_pts[3]), canvas.canvasToWidget(m_pts[2]));
    for (int i = 0; i < 4; ++i) {
        const QPointF c = canvas.canvasToWidget(m_pts[i]);
        const bool endpoint = (i == 0 || i == 3);
        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(endpoint ? QColor(255, 255, 255) : QColor(0, 120, 215));
        painter.drawEllipse(c, 4, 4);
    }
    painter.restore();
}
