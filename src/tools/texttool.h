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

    void commitText(CanvasWidget &canvas);

private:
    bool m_editing = false;
    QPointF m_textPos;
    QString m_text;
    QFont m_font{"Sans", 24};
    bool m_bold = false;
    bool m_italic = false;
};
