#pragma once

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QPoint>
#include <QTransform>

class Document;
class Tool;

class CanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget *parent = nullptr);

    void setDocument(Document *doc);
    Document *document() const { return m_document; }

    void setCurrentTool(Tool *tool);
    Tool *currentTool() const { return m_currentTool; }

    // View
    double zoom() const { return m_zoom; }
    void setZoom(double zoom);
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToRect(const QRect &canvasRect);   // fit a document rectangle to the view
    void zoomToActual();
    void resetToDefaultView();
    // Centres the image in the viewport at the current zoom.
    void centerView();
    QPointF pan() const { return m_pan; }
    // Any explicit pan is a user gesture: stop auto-centring from then on.
    void setPan(const QPointF &pan) { m_pan = pan; m_autoCenter = false; update(); }

    // Coordinate conversion
    QPointF widgetToCanvas(const QPointF &widgetPos) const;
    QPointF canvasToWidget(const QPointF &canvasPos) const;

    // Rendering
    void updateCanvas();
    void setShowGrid(bool show) { m_showGrid = show; update(); }
    bool showGrid() const { return m_showGrid; }
    void setShowRulers(bool show);

    // Measurement unit for the rulers and cursor position.
    enum class Unit { Pixels, Inches, Centimeters };
    void setUnit(Unit u) { m_unit = u; update(); }
    Unit unit() const { return m_unit; }
    double dpi() const { return m_dpi; }
    void setDpi(double d) { m_dpi = d > 0 ? d : 96.0; update(); }

    // Backdrop around the image; follows the active colour scheme.
    void setBackdropColor(const QColor &c) { m_backdrop = c; update(); }
    // Re-reads canvas-related preferences (transparency checkerboard brightness).
    void reloadSettings();
    bool showRulers() const;
    int rulerSize() const { return m_showRulers ? 20 : 0; }

signals:
    void zoomChanged(double zoom);
    void cursorPositionChanged(const QPoint &pos);
    void canvasModified();
    void deleteSelectionRequested();
    void fillSelectionRequested();
    void selectionContextMenuRequested(const QPoint &globalPos);
    void tabletDetected();   // emitted the first time a stylus is used

protected:
    bool event(QEvent *event) override;   // claim ShortcutOverride while a tool types
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void tabletEvent(QTabletEvent *event) override;

private:
    void drawCheckerboard(QPainter &painter, const QRect &rect);
    void drawSelectionMarching(QPainter &painter);
    void updateMarchingAnts();

    Document *m_document = nullptr;
    Tool *m_currentTool = nullptr;

    double m_zoom = 1.0;
    QPointF m_pan{0, 0};
    // While true the image re-centres on every resize, so the startup view is
    // centred whatever the final window size is (maximised or restored). Any
    // user pan turns it off; resetToDefaultView turns it back on.
    bool m_autoCenter = true;

    bool m_isPanning = false;
    bool m_spaceDown = false;   // Space held: pan-drag with any tool (Paint.NET)
    QPoint m_lastPanPos;

    bool m_showGrid = false;
    QColor m_backdrop{150, 150, 150};
    int m_checkerBrightness = 80;   // 0..100, from Settings > Canvas
    Unit m_unit = Unit::Pixels;
    double m_dpi = 96.0;
    bool m_showRulers = true;

    // Marching ants
    QTimer m_marchingTimer;
    int m_marchingOffset = 0;

    // Cached render
    QImage m_cachedRender;
    bool m_cacheValid = false;

    // Tablet pressure
    double m_pressure = 1.0;
    bool m_tabletSeen = false;   // a stylus event has arrived at least once
    bool m_penInContact = false; // stylus tip is touching (pressure > 0), not just hovering
};
