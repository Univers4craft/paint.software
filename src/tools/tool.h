#pragma once

#include <QPointF>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QString>
#include <QIcon>

class CanvasWidget;
class Document;
class QImage;

enum class ToolType {
    Brush,
    Eraser,
    Pencil,
    Fill,
    ColorPicker,
    Recolor,
    RectSelection,
    EllipseSelection,
    LassoSelection,
    MagicWand,
    Move,
    MoveSelection,
    Zoom,
    Pan,
    Text,
    Shape,
    Line,
    Gradient,
    CloneStamp
};

class Tool {
public:
    virtual ~Tool() = default;

    virtual ToolType type() const = 0;
    virtual QString name() const = 0;
    virtual QCursor cursor() const { return Qt::CrossCursor; }

    virtual void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) = 0;
    virtual void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) = 0;
    virtual void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) = 0;
    virtual void keyPressEvent(QKeyEvent *event, CanvasWidget &canvas) { Q_UNUSED(event); Q_UNUSED(canvas); }
    virtual void keyReleaseEvent(QKeyEvent *event, CanvasWidget &canvas) { Q_UNUSED(event); Q_UNUSED(canvas); }
    virtual void drawOverlay(QPainter &painter, const CanvasWidget &canvas) { Q_UNUSED(painter); Q_UNUSED(canvas); }

    // Tool options
    int brushSize() const { return m_brushSize; }
    void setBrushSize(int size) { m_brushSize = qBound(1, size, 500); }

    int hardness() const { return m_hardness; }
    void setHardness(int hardness) { m_hardness = qBound(0, hardness, 100); }

    int opacity() const { return m_opacity; }
    void setOpacity(int opacity) { m_opacity = qBound(1, opacity, 100); }

    bool antialiased() const { return m_antialiased; }
    void setAntialiased(bool aa) { m_antialiased = aa; }

    double pressure() const { return m_pressure; }
    void setPressure(double p) { m_pressure = qBound(0.0, p, 1.0); }

    int tolerance() const { return m_tolerance; }
    void setTolerance(int t) { m_tolerance = qBound(0, t, 255); }

    // Dab spacing as a percentage of brush size (smaller = smoother strokes).
    int spacing() const { return m_spacing; }
    void setSpacing(int s) { m_spacing = qBound(1, s, 200); }

    // Brush blend mode index (maps to QPainter composition modes in the tool).
    int blendMode() const { return m_blendMode; }
    void setBlendMode(int m) { m_blendMode = m; }

protected:
    // Selection-awareness helpers shared by all painting tools.
    // Clips a painter (drawing into a layer image, image space) to the active
    // selection so strokes never spill outside it. No-op if nothing is selected.
    static void clipToSelection(QPainter &painter, Document *doc);
    // For pixel-level tools: true if (x,y) may be edited given the selection.
    static bool selectionAllows(Document *doc, int x, int y);
    // Masks a freshly produced full-layer image so only selected pixels differ
    // from the original. Used by tools that rebuild the whole image.
    static QImage maskEditToSelection(Document *doc, const QImage &before, const QImage &after);

    int m_brushSize = 10;
    int m_hardness = 100;
    int m_opacity = 100;
    bool m_antialiased = true;
    double m_pressure = 1.0;
    int m_tolerance = 32;
    int m_spacing = 15;
    int m_blendMode = 0;
};
