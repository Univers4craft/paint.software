#pragma once
#include "tool.h"
#include <QFont>

class TextTool : public Tool {
public:
    ToolType type() const override { return ToolType::Text; }
    QString name() const override { return "Text"; }

    void mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) override;
    void keyPressEvent(QKeyEvent *event, CanvasWidget &canvas) override;
    void drawOverlay(QPainter &painter, const CanvasWidget &canvas) override;

    void setFont(const QFont &font) { m_font = font; }
    QFont font() const { return m_font; }
    void setFontFamily(const QString &family) { m_font.setFamily(family); }
    QString fontFamily() const { return m_font.family(); }
    void setFontSize(int pt) { m_font.setPointSize(qMax(1, pt)); }
    int fontSize() const { return m_font.pointSize() > 0 ? m_font.pointSize() : 24; }
    void setBold(bool b) { m_bold = b; }
    bool bold() const { return m_bold; }
    void setItalic(bool i) { m_italic = i; }
    bool italic() const { return m_italic; }
    void setUnderline(bool u) { m_underline = u; }
    bool underline() const { return m_underline; }

    void commitText(CanvasWidget &canvas);

private:
    // The font actually used for rendering: base family/size plus the B/I/U flags.
    QFont effectiveFont() const;

    bool m_editing = false;
    QPointF m_textPos;
    QString m_text;
    QFont m_font{"Sans", 24};
    bool m_bold = false;
    bool m_italic = false;
    bool m_underline = false;
};
