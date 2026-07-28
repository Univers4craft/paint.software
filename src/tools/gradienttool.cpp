#include "gradienttool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QConicalGradient>
#include <cmath>

void GradientTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_drawing = true;
    m_useRight = (event->button() == Qt::RightButton);
    m_startPos = canvasPos;
    m_endPos = canvasPos;
    m_beforeImage = layer->image().copy();
}

void GradientTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_endPos = canvasPos;
    applyGradient(canvas);
}

void GradientTool::mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    m_endPos = canvasPos;
    applyGradient(canvas);
    canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Gradient");
}

namespace {
QGradient::Spread spreadFor(GradientRepeat r) {
    switch (r) {
    case GradientRepeat::Repeat:  return QGradient::RepeatSpread;
    case GradientRepeat::Reflect: return QGradient::ReflectSpread;
    case GradientRepeat::None:
    default:                      return QGradient::PadSpread;
    }
}

// Folds a raw gradient parameter into [0,1] according to the repeat mode, for
// the hand-rolled gradients (Diamond, Spiral) that don't go through QGradient.
double foldT(double t, GradientRepeat r) {
    switch (r) {
    case GradientRepeat::Repeat:
        t = t - std::floor(t);
        break;
    case GradientRepeat::Reflect: {
        t = std::fabs(t);
        double f = std::fmod(t, 2.0);
        t = (f <= 1.0) ? f : 2.0 - f;
        break;
    }
    case GradientRepeat::None:
    default:
        t = std::min(1.0, std::max(0.0, t));
        break;
    }
    return t;
}
}

void GradientTool::applyGradient(CanvasWidget &canvas) {
    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer) return;

    layer->setImage(m_beforeImage.copy());
    QPainter painter(&layer->image());
    if (m_antialiased) painter.setRenderHint(QPainter::Antialiasing);
    clipToSelection(painter, doc);
    painter.setOpacity(m_opacity / 100.0);

    // Right-button drag swaps the primary/secondary roles, like the other tools.
    QColor base = m_useRight ? doc->secondaryColor() : doc->primaryColor();
    QColor other = m_useRight ? doc->primaryColor() : doc->secondaryColor();

    QColor c1, c2;
    if (m_transparencyMode) {
        // Only the alpha varies: opaque base colour → fully transparent. Replace
        // the destination (Source composition) so the ramp actually writes the
        // alpha channel, revealing transparency even over opaque content.
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        c1 = base;
        c2 = base;
        c2.setAlpha(0);
    } else {
        c1 = base;
        c2 = other;
    }

    const QGradient::Spread spread = spreadFor(m_repeatMode);

    switch (m_gradientType) {
    case GradientType::Linear: {
        QLinearGradient gradient(m_startPos, m_endPos);
        gradient.setColorAt(0, c1);
        gradient.setColorAt(1, c2);
        gradient.setSpread(spread);
        painter.fillRect(layer->image().rect(), gradient);
        break;
    }
    case GradientType::LinearReflected: {
        // Mirrors the gradient at the endpoint (start acts as the fixed anchor).
        // Reflect is its defining behaviour; an explicit repeat mode overrides it.
        QLinearGradient gradient(m_startPos, m_endPos);
        gradient.setColorAt(0, c1);
        gradient.setColorAt(1, c2);
        gradient.setSpread(m_repeatMode == GradientRepeat::None ? QGradient::ReflectSpread
                                                                : spread);
        painter.fillRect(layer->image().rect(), gradient);
        break;
    }
    case GradientType::Radial: {
        double radius = std::sqrt(std::pow(m_endPos.x()-m_startPos.x(),2) + std::pow(m_endPos.y()-m_startPos.y(),2));
        QRadialGradient gradient(m_startPos, radius);
        gradient.setColorAt(0, c1);
        gradient.setColorAt(1, c2);
        gradient.setSpread(spread);
        painter.fillRect(layer->image().rect(), gradient);
        break;
    }
    case GradientType::Conical: {
        double angle = std::atan2(m_endPos.y()-m_startPos.y(), m_endPos.x()-m_startPos.x()) * 180.0 / M_PI;
        QConicalGradient gradient(m_startPos, angle);
        gradient.setColorAt(0, c1);
        gradient.setColorAt(0.5, c2);
        gradient.setColorAt(1, c1);
        painter.fillRect(layer->image().rect(), gradient);
        break;
    }
    case GradientType::Diamond: {
        // True diamond: colour follows the L1 distance in the axis/perpendicular
        // frame, so iso-lines are diamonds centred on the start point.
        const double dxE = m_endPos.x() - m_startPos.x();
        const double dyE = m_endPos.y() - m_startPos.y();
        const double len = std::sqrt(dxE * dxE + dyE * dyE);
        QImage grad(layer->image().size(), QImage::Format_ARGB32);
        if (len < 1e-6) {
            grad.fill(c1.rgba());
        } else {
            const double ux = dxE / len, uy = dyE / len;   // axis unit vector
            for (int y = 0; y < grad.height(); ++y) {
                QRgb *line = reinterpret_cast<QRgb*>(grad.scanLine(y));
                for (int x = 0; x < grad.width(); ++x) {
                    const double px = x - m_startPos.x();
                    const double py = y - m_startPos.y();
                    const double u = (px * ux + py * uy) / len;    // along axis
                    const double v = (-px * uy + py * ux) / len;   // perpendicular
                    double t = foldT(std::abs(u) + std::abs(v), m_repeatMode);
                    line[x] = qRgba(
                        int(c1.red()   + (c2.red()   - c1.red())   * t),
                        int(c1.green() + (c2.green() - c1.green()) * t),
                        int(c1.blue()  + (c2.blue()  - c1.blue())  * t),
                        int(c1.alpha() + (c2.alpha() - c1.alpha()) * t));
                }
            }
        }
        painter.drawImage(0, 0, grad);
        break;
    }
    case GradientType::Spiral: {
        // Angular sweep combined with radial progression: the colour winds
        // outward from the start point like a spiral.
        const double dxE = m_endPos.x() - m_startPos.x();
        const double dyE = m_endPos.y() - m_startPos.y();
        const double radius = std::sqrt(dxE * dxE + dyE * dyE);
        QImage grad(layer->image().size(), QImage::Format_ARGB32);
        if (radius < 1e-6) {
            grad.fill(c1.rgba());
        } else {
            const double baseAngle = std::atan2(dyE, dxE);
            for (int y = 0; y < grad.height(); ++y) {
                QRgb *line = reinterpret_cast<QRgb*>(grad.scanLine(y));
                for (int x = 0; x < grad.width(); ++x) {
                    const double px = x - m_startPos.x();
                    const double py = y - m_startPos.y();
                    const double ang = std::atan2(py, px) - baseAngle;    // radians
                    const double dist = std::sqrt(px * px + py * py) / radius;
                    // One full turn per radius: angle contributes a fractional
                    // winding, distance drives the outward march.
                    double raw = dist + ang / (2.0 * M_PI);
                    double t = foldT(raw, m_repeatMode);
                    line[x] = qRgba(
                        int(c1.red()   + (c2.red()   - c1.red())   * t),
                        int(c1.green() + (c2.green() - c1.green()) * t),
                        int(c1.blue()  + (c2.blue()  - c1.blue())  * t),
                        int(c1.alpha() + (c2.alpha() - c1.alpha()) * t));
                }
            }
        }
        painter.drawImage(0, 0, grad);
        break;
    }
    }
}
