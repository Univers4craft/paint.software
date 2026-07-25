#include "canvaswidget.h"
#include "core/document.h"
#include "tools/tool.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QTabletEvent>
#include <QSettings>
#include <cmath>

bool CanvasWidget::event(QEvent *event)
{
    // Qt matches single-key shortcuts (B, E, S…) before the key reaches
    // keyPressEvent, so while the Text tool is typing those letters would trigger
    // a tool switch and never be typed — ~60% of the alphabet vanished. Accepting
    // ShortcutOverride tells Qt to deliver the key as a normal press instead.
    if (event->type() == QEvent::ShortcutOverride && m_currentTool) {
        auto *ke = static_cast<QKeyEvent *>(event);
        const bool ctrlAlt = ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
        if ((m_currentTool->wantsKeyInput() && !ctrlAlt)
            || m_currentTool->wantsCommitKey(ke->key())) {
            event->accept();
            return true;
        }
    }
    return QWidget::event(event);
}

CanvasWidget::CanvasWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TabletTracking);

    connect(&m_marchingTimer, &QTimer::timeout, this, &CanvasWidget::updateMarchingAnts);
    m_marchingTimer.start(150);
}

void CanvasWidget::setDocument(Document *doc) {
    m_document = doc;
    if (doc) {
        connect(doc, &Document::documentChanged, this, [this]() {
            m_cacheValid = false;
            update();
        });
        connect(doc, &Document::layersChanged, this, [this]() {
            m_cacheValid = false;
            update();
        });
        connect(doc, &Document::selectionChanged, this, [this]() { update(); });
    }
    m_cacheValid = false;
    update();
}

void CanvasWidget::setCurrentTool(Tool *tool) {
    if (m_currentTool && m_currentTool != tool)
        m_currentTool->deactivate(*this);   // commit any pending edit before switching
    m_currentTool = tool;
}

void CanvasWidget::setZoom(double zoom) {
    m_zoom = qBound(0.01, zoom, 64.0);
    m_cacheValid = false;
    emit zoomChanged(m_zoom);
    update();
}

void CanvasWidget::setShowRulers(bool show) {
    m_showRulers = show;
    update();
}

bool CanvasWidget::showRulers() const {
    return m_showRulers;
}

void CanvasWidget::zoomIn() {
    double newZoom = m_zoom;
    if (m_zoom < 1.0) newZoom = m_zoom * 1.5;
    else if (m_zoom < 2.0) newZoom = m_zoom + 0.25;
    else if (m_zoom < 8.0) newZoom = m_zoom + 1.0;
    else newZoom = m_zoom * 1.5;
    setZoom(newZoom);
}

void CanvasWidget::zoomOut() {
    double newZoom = m_zoom;
    if (m_zoom > 8.0) newZoom = m_zoom / 1.5;
    else if (m_zoom > 2.0) newZoom = m_zoom - 1.0;
    else if (m_zoom > 1.0) newZoom = m_zoom - 0.25;
    else newZoom = m_zoom / 1.5;
    setZoom(newZoom);
}

void CanvasWidget::zoomToFit() {
    if (!m_document) return;
    int rs = m_showRulers ? 20 : 0;
    double zx = (double)(width() - 20 - rs) / m_document->width();
    double zy = (double)(height() - 20 - rs) / m_document->height();
    setZoom(std::min(zx, zy));
    m_pan = QPointF(
        (width() - rs - m_document->width() * m_zoom) / 2.0,
        (height() - rs - m_document->height() * m_zoom) / 2.0
    );
    update();
}

void CanvasWidget::zoomToRect(const QRect &canvasRect) {
    if (!m_document || canvasRect.isEmpty()) return;
    const int rs = m_showRulers ? 20 : 0;
    const double margin = 24.0;
    const double zx = (width() - rs - margin * 2) / canvasRect.width();
    const double zy = (height() - rs - margin * 2) / canvasRect.height();
    setZoom(std::min(zx, zy));
    // Centre the rectangle in the viewport.
    m_pan = QPointF(
        (width() - rs - canvasRect.width() * m_zoom) / 2.0 - canvasRect.x() * m_zoom,
        (height() - rs - canvasRect.height() * m_zoom) / 2.0 - canvasRect.y() * m_zoom
    );
    m_autoCenter = false;   // framed on a specific rect, not on the whole image
    update();
}

void CanvasWidget::zoomToActual() {
    setZoom(1.0);
    if (m_document) {
        int rs = m_showRulers ? 20 : 0;
        m_pan = QPointF(
            (width() - rs - m_document->width()) / 2.0,
            (height() - rs - m_document->height()) / 2.0
        );
    }
    update();
}

void CanvasWidget::centerView() {
    if (!m_document) return;

    // paint.net opens with the canvas centred, not pinned to the top-left. The
    // image is painted at m_pan + (rs, rs), so centring it inside the area right
    // of the rulers means panning by half the leftover space.
    // No margin clamp here: it would push a snugly-fitting image off-centre, and
    // an image larger than the viewport is better centred (overflowing evenly)
    // than pinned to the left.
    const int rs = m_showRulers ? 20 : 0;
    const qreal imgW = m_document->width() * m_zoom;
    const qreal imgH = m_document->height() * m_zoom;
    m_pan = QPointF((width() - rs - imgW) / 2.0, (height() - rs - imgH) / 2.0);
    update();
}

void CanvasWidget::resetToDefaultView() {
    if (!m_document) return;

    const int rs = m_showRulers ? 20 : 0;
    const qreal margin = 40.0;
    const qreal availableWidth = std::max(1.0, width() - rs - margin * 2.0);
    const qreal availableHeight = std::max(1.0, height() - rs - margin * 2.0);
    const qreal fitZoom = std::min(availableWidth / m_document->width(), availableHeight / m_document->height());

    setZoom(std::min(1.0, fitZoom));
    m_autoCenter = true;   // follow the window again until the user pans
    centerView();
}

QPointF CanvasWidget::widgetToCanvas(const QPointF &widgetPos) const {
    int rs = m_showRulers ? 20 : 0;
    return (widgetPos - QPointF(rs, rs) - m_pan) / m_zoom;
}

QPointF CanvasWidget::canvasToWidget(const QPointF &canvasPos) const {
    int rs = m_showRulers ? 20 : 0;
    return canvasPos * m_zoom + m_pan + QPointF(rs, rs);
}

void CanvasWidget::updateCanvas() {
    m_cacheValid = false;
    update();
}

void CanvasWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_zoom < 2.0);

    // Backdrop around the image (paint.net's "editing window"), scheme-aware
    painter.fillRect(rect(), m_backdrop);

    if (!m_document) return;

    // Ruler offset
    int rs = m_showRulers ? 20 : 0;

    // Canvas area
    QRectF canvasRect(m_pan.x() + rs, m_pan.y() + rs,
                      m_document->width() * m_zoom,
                      m_document->height() * m_zoom);

    // Checkerboard for transparency
    drawCheckerboard(painter, canvasRect.toRect());

    // Render document
    if (!m_cacheValid) {
        m_cachedRender = m_document->flattenVisible();
        m_cacheValid = true;
    }
    painter.drawImage(canvasRect, m_cachedRender);

    // Grid at high zoom
    if (m_showGrid && m_zoom >= 4.0) {
        painter.setPen(QPen(QColor(0, 0, 0, 40), 1));
        double startX = m_pan.x() + rs;
        double startY = m_pan.y() + rs;
        for (int x = 0; x <= m_document->width(); ++x) {
            double wx = startX + x * m_zoom;
            painter.drawLine(QPointF(wx, startY), QPointF(wx, startY + m_document->height() * m_zoom));
        }
        for (int y = 0; y <= m_document->height(); ++y) {
            double wy = startY + y * m_zoom;
            painter.drawLine(QPointF(startX, wy), QPointF(startX + m_document->width() * m_zoom, wy));
        }
    }

    // Canvas border
    painter.setPen(QPen(Qt::darkGray, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(canvasRect.adjusted(-1, -1, 0, 0));

    // Selection marching ants
    if (m_document->selection().hasSelection()) {
        drawSelectionMarching(painter);
    }

    // Tool overlay
    if (m_currentTool) {
        m_currentTool->drawOverlay(painter, *this);
    }

    // Rulers (drawn on top of everything, like Paint.NET)
    if (m_showRulers && m_document) {
        const int rulerSize = 20;
        QFont rulerFont("Sans", 7);
        painter.setFont(rulerFont);

        // Theme-aware colours: light strip on light backdrop, dark on dark.
        const bool dark = m_backdrop.lightness() < 110;
        const QColor rulerBg   = dark ? QColor(58, 58, 58)   : QColor(255, 255, 255);
        const QColor rulerLine = dark ? QColor(90, 90, 90)   : QColor(180, 180, 180);
        const QColor tickCol   = dark ? QColor(190, 190, 190) : QColor(100, 100, 100);
        const QColor textCol   = dark ? QColor(220, 220, 220) : QColor(60, 60, 60);

        // Backgrounds
        painter.fillRect(QRect(rulerSize, 0, width() - rulerSize, rulerSize), rulerBg);
        painter.fillRect(QRect(0, rulerSize, rulerSize, height() - rulerSize), rulerBg);
        painter.fillRect(QRect(0, 0, rulerSize, rulerSize), rulerBg);
        painter.setPen(QPen(rulerLine, 1));
        painter.drawLine(rulerSize, rulerSize, width(), rulerSize);
        painter.drawLine(rulerSize, rulerSize, rulerSize, height());
        painter.drawRect(QRect(0, 0, rulerSize, rulerSize));

        // Pixels-per-unit for the chosen measurement unit.
        const double pxPerUnit = (m_unit == Unit::Inches)      ? m_dpi
                               : (m_unit == Unit::Centimeters) ? m_dpi / 2.54
                                                               : 1.0;
        // On-screen pixels per unit, used to pick a readable tick step.
        const double screenPerUnit = pxPerUnit * m_zoom;
        // Choose a "nice" major step (in units) so labels are ~60px apart.
        auto niceStep = [&](double minScreen) -> double {
            static const double steps[] = {0.1, 0.25, 0.5, 1, 2, 5, 10, 25, 50, 100, 200, 500, 1000};
            for (double s : steps)
                if (s * screenPerUnit >= minScreen) return s;
            return 2000.0;
        };
        const double majorStep = niceStep(58.0);
        const double smallStep = majorStep / 5.0;   // 5 minor ticks per major

        // Label formatting: pixels are integers, in/cm show up to 2 decimals.
        auto fmt = [&](double unitVal) -> QString {
            if (m_unit == Unit::Pixels) return QString::number(qRound(unitVal));
            QString s = QString::number(unitVal, 'f', 2);
            while (s.contains('.') && (s.endsWith('0') || s.endsWith('.'))) s.chop(1);
            return s;
        };

        // Visible canvas range, in the chosen unit (ruler is "infinite":
        // negative values before the origin, values beyond the image).
        const double leftU  = ((-m_pan.x()) / m_zoom) / pxPerUnit;
        const double rightU = ((width()  - rulerSize - m_pan.x()) / m_zoom) / pxPerUnit;
        const double topU   = ((-m_pan.y()) / m_zoom) / pxPerUnit;
        const double botU   = ((height() - rulerSize - m_pan.y()) / m_zoom) / pxPerUnit;

        auto unitToWx = [&](double u) { return m_pan.x() + rulerSize + u * pxPerUnit * m_zoom; };
        auto unitToWy = [&](double u) { return m_pan.y() + rulerSize + u * pxPerUnit * m_zoom; };
        auto isMultiple = [](double v, double step) {
            return std::abs(v / step - std::round(v / step)) < 1e-6;
        };

        painter.setPen(QPen(tickCol, 1));
        const double startX = std::floor(leftU / smallStep) * smallStep;
        for (double u = startX; u <= rightU + smallStep; u += smallStep) {
            const double wx = unitToWx(u);
            if (wx < rulerSize || wx > width()) continue;
            if (isMultiple(u, majorStep)) {
                painter.setPen(QPen(tickCol, 1));
                painter.drawLine(QPointF(wx, 0), QPointF(wx, rulerSize));
                painter.setPen(textCol);
                painter.drawText(QPointF(wx + 2, rulerSize - 4), fmt(u));
            } else {
                painter.setPen(QPen(tickCol, 1));
                painter.drawLine(QPointF(wx, rulerSize * 0.7), QPointF(wx, rulerSize));
            }
        }

        const double startY = std::floor(topU / smallStep) * smallStep;
        for (double u = startY; u <= botU + smallStep; u += smallStep) {
            const double wy = unitToWy(u);
            if (wy < rulerSize || wy > height()) continue;
            if (isMultiple(u, majorStep)) {
                painter.setPen(QPen(tickCol, 1));
                painter.drawLine(QPointF(0, wy), QPointF(rulerSize, wy));
                painter.setPen(textCol);
                painter.save();
                painter.translate(rulerSize - 4, wy + 2);
                painter.rotate(90);
                painter.drawText(0, 0, fmt(u));
                painter.restore();
            } else {
                painter.setPen(QPen(tickCol, 1));
                painter.drawLine(QPointF(rulerSize * 0.7, wy), QPointF(rulerSize, wy));
            }
        }

        // Cursor position indicator on the rulers.
        painter.setPen(QPen(QColor(255, 60, 60, 200), 1));
        QPoint cursorPos = mapFromGlobal(QCursor::pos());
        if (cursorPos.x() >= rulerSize && cursorPos.x() < width())
            painter.drawLine(cursorPos.x(), 0, cursorPos.x(), rulerSize);
        if (cursorPos.y() >= rulerSize && cursorPos.y() < height())
            painter.drawLine(0, cursorPos.y(), rulerSize, cursorPos.y());
    }
}

void CanvasWidget::reloadSettings() {
    QSettings s("PaintDali", "PaintDali");
    m_checkerBrightness = s.value("canvas/checkerBrightness", 80).toInt();
    update();
}

void CanvasWidget::drawCheckerboard(QPainter &painter, const QRect &rect) {
    const int gridSize = 8;
    // Brightness slider (Settings > Canvas) scales the checkerboard: 100 = white
    // squares, lower values darken both tones.
    const int base = qBound(0, 155 + m_checkerBrightness, 255);
    const int alt = qBound(0, base - 51, 255);
    QColor c1(base, base, base);
    QColor c2(alt, alt, alt);

    painter.save();
    painter.setClipRect(rect);
    int startX = rect.left() - (rect.left() % (gridSize * 2));
    int startY = rect.top() - (rect.top() % (gridSize * 2));

    for (int y = startY; y < rect.bottom(); y += gridSize) {
        for (int x = startX; x < rect.right(); x += gridSize) {
            bool dark = ((x / gridSize) + (y / gridSize)) % 2;
            painter.fillRect(x, y, gridSize, gridSize, dark ? c2 : c1);
        }
    }
    painter.restore();
}

void CanvasWidget::drawSelectionMarching(QPainter &painter) {
    if (!m_document) return;

    const Selection &sel = m_document->selection();
    QRegion region = sel.region();
    if (region.isEmpty()) return;

    painter.save();
    int rs = m_showRulers ? 20 : 0;
    painter.translate(m_pan + QPointF(rs, rs));
    painter.scale(m_zoom, m_zoom);

    QPen pen1(Qt::white, 1.0 / m_zoom);
    QPen pen2(Qt::black, 1.0 / m_zoom, Qt::DashLine);
    QVector<qreal> dashPattern;
    dashPattern << 4 << 4;
    pen2.setDashPattern(dashPattern);
    pen2.setDashOffset(m_marchingOffset);

    // Build ONE outline for the whole region. Adding each region rect as its own
    // subpath and stroking it outlined every internal scanline rectangle, which
    // filled non-rectangular selections (ellipse, lasso) with static instead of
    // just tracing the perimeter (issue #15). simplified() merges the rects so
    // only the true boundary is stroked.
    QPainterPath path;
    path.addRegion(region);
    path = path.simplified();

    painter.setPen(pen1);
    painter.drawPath(path);
    painter.setPen(pen2);
    painter.drawPath(path);
    painter.restore();
}

void CanvasWidget::updateMarchingAnts() {
    m_marchingOffset = (m_marchingOffset + 1) % 8;
    if (m_document && m_document->selection().hasSelection())
        update();
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    // Pan with the middle button, or with the left button while Space is held —
    // Paint.NET's shortcut. Alt is deliberately NOT a pan modifier: it is the
    // selection-subtract key and the two would collide.
    if (event->button() == Qt::MiddleButton
        || (event->button() == Qt::LeftButton && m_spaceDown)) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // Only a *bare* right-click opens the selection context menu. Alt+right and
    // Ctrl+right are selection-combine gestures (intersect / invert) and must
    // reach the active tool instead.
    if (event->button() == Qt::RightButton && event->modifiers() == Qt::NoModifier
        && m_document && m_document->selection().hasSelection()) {
        QPoint canvasPos = widgetToCanvas(event->position()).toPoint();
        if (m_document->selection().isSelected(canvasPos.x(), canvasPos.y())) {
            emit selectionContextMenuRequested(event->globalPosition().toPoint());
            event->accept();
            return;
        }
    }

    if (m_currentTool && m_document) {
        setFocus(Qt::MouseFocusReason);   // so Enter/Escape reach the tool (Line/Curve commit)
        QPointF canvasPos = widgetToCanvas(event->position());
        m_currentTool->mousePressEvent(canvasPos, event, *this);
        m_cacheValid = false;
        update();
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_pan += QPointF(delta);
        m_autoCenter = false;   // the user placed the view; leave it alone
        m_lastPanPos = event->pos();
        update();
        return;
    }

    QPointF canvasPos = widgetToCanvas(event->position());
    emit cursorPositionChanged(canvasPos.toPoint());

    if (m_currentTool && m_document) {
        m_currentTool->mouseMoveEvent(canvasPos, event, *this);
        m_cacheValid = false;
        update();
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (m_isPanning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (m_currentTool && m_document) {
        m_currentTool->mouseReleaseEvent(widgetToCanvas(event->position()), event, *this);
        m_cacheValid = false;
        update();
        emit canvasModified();
    }
}

void CanvasWidget::wheelEvent(QWheelEvent *event) {
    QPointF oldCanvasPos = widgetToCanvas(event->position());
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    setZoom(m_zoom * factor);

    QPointF newWidgetPos = canvasToWidget(oldCanvasPos);
    m_pan += event->position() - newWidgetPos;
    m_autoCenter = false;   // wheel zoom anchors on the cursor; keep that anchor
    update();
}

void CanvasWidget::keyPressEvent(QKeyEvent *event) {
    // Space arms pan-drag for any tool (unless the Text tool is capturing keys).
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()
        && !(m_currentTool && m_currentTool->wantsKeyInput())) {
        if (!m_spaceDown) {
            m_spaceDown = true;
            if (!m_isPanning) setCursor(Qt::OpenHandCursor);
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete && m_document && m_document->selection().hasSelection()) {
        emit deleteSelectionRequested();
        event->accept();
        return;
    }
    // Backspace fills the selection (paint.net). Handled here, like Delete, so it
    // fires reliably whenever the canvas has focus — but not while the Text tool
    // is typing, where Backspace must delete a character instead.
    if (event->key() == Qt::Key_Backspace && m_document
        && !(m_currentTool && m_currentTool->wantsKeyInput())) {
        emit fillSelectionRequested();
        event->accept();
        return;
    }

    if (m_currentTool) {
        m_currentTool->keyPressEvent(event, *this);
        m_cacheValid = false;
        update();  // reflect tool state (e.g. text being typed) immediately
    }
    QWidget::keyPressEvent(event);
}

void CanvasWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat() && m_spaceDown) {
        m_spaceDown = false;
        if (!m_isPanning && m_currentTool) setCursor(m_currentTool->cursor());
        event->accept();
        return;
    }
    if (m_currentTool) {
        m_currentTool->keyReleaseEvent(event, *this);
        update();
    }
    QWidget::keyReleaseEvent(event);
}

void CanvasWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // The startup view has to be centred whatever the final window size turns out
    // to be: the window manager maximises us *after* the first layout pass, so
    // centring only at construction leaves the image off-centre. Re-centring on
    // every resize until the user pans keeps it right, maximised or restored.
    if (m_autoCenter) centerView();
}

void CanvasWidget::tabletEvent(QTabletEvent *event) {
    // Record pressure, then ignore so Qt still synthesises the compatibility
    // mouse press/move/release that actually drive the tool. Accepting here would
    // swallow those and the stylus would update pressure but never draw.
    m_pressure = event->pressure() > 0.0 ? event->pressure() : m_pressure;
    if (m_currentTool)
        m_currentTool->setPressure(m_pressure);
    event->ignore();
}
