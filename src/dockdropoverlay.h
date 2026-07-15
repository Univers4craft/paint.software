#pragma once

#include <QWidget>

class QDockWidget;

// Semi-transparent overlay displayed over a docked panel while another panel
// is being dragged. Shows five drop zones (Top / Bottom / Left / Right / Center)
// as large, easy-to-hit areas with directional arrows.
// The overlay is purely visual: it has WA_TransparentForMouseEvents so it never
// interferes with Qt's own drag handling. Zone detection is done by the caller
// (MainWindow::computeDropZone) by comparing the cursor position against the
// rectangles  returned by zoneRect().
class DockDropOverlay : public QWidget
{
    Q_OBJECT
public:
    enum Zone { None, Top, Bottom, Left, Right, Center };

    explicit DockDropOverlay(QWidget *parent = nullptr);

    // Move and resize the overlay to cover 'target', then show it.
    void updateTarget(QDockWidget *target, Zone hoveredZone);

    void hideOverlay();

    QDockWidget *targetDock()  const { return m_targetDock; }
    Zone         currentZone() const { return m_zone; }

    // Returns the rectangle (in overlay-local coordinates) for the given zone.
    QRect zoneRect(Zone zone) const;

    // Static version – computes zone geometry for an arbitrary widget size.
    // Used by callers that should not instantiate a full overlay object.
    static QRect zoneRectForSize(Zone zone, const QSize &size);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QDockWidget *m_targetDock = nullptr;
    Zone         m_zone       = None;

    void drawArrow(QPainter &p, const QRect &r, Zone zone, bool active);
};
