#include "texttool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFontMetricsF>
#include <algorithm>

void TextTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;

    if (m_editing && !m_text.isEmpty()) {
        commitText(canvas);
    }

    m_editing = true;
    m_textPos = canvasPos;
    m_text.clear();
}

void TextTool::mouseMoveEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}
void TextTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}

void TextTool::keyPressEvent(QKeyEvent *event, CanvasWidget &canvas) {
    if (!m_editing) return;

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() & Qt::ShiftModifier) {
            m_text += '\n';
        } else {
            commitText(canvas);
        }
    } else if (event->key() == Qt::Key_Backspace) {
        if (!m_text.isEmpty()) m_text.chop(1);
    } else if (event->key() == Qt::Key_Escape) {
        m_editing = false;
        m_text.clear();
    } else if (!event->text().isEmpty()) {
        m_text += event->text();
    }
}

void TextTool::insertText(const QString &text, CanvasWidget &canvas) {
    if (!m_editing || text.isEmpty()) return;
    // Normalise line endings so pasted CRLF / lone CR behave like typed Enter.
    QString clean = text;
    clean.replace("\r\n", "\n").replace('\r', '\n');
    m_text += clean;
    canvas.update();
}

double TextTool::alignOffset(const QString &line, const QFontMetricsF &fm) const {
    if (m_align == Align::Left) return 0.0;
    const double w = fm.horizontalAdvance(line);
    return (m_align == Align::Center) ? -w / 2.0 : -w;
}

QFont TextTool::effectiveFont() const {
    QFont f = m_font;
    f.setBold(m_bold);
    f.setItalic(m_italic);
    f.setUnderline(m_underline);
    f.setStrikeOut(m_strikeout);
    return f;
}

void TextTool::drawOverlay(QPainter &painter, const CanvasWidget &canvas) {
    if (!m_editing) return;

    painter.save();
    QPointF wPos = canvas.canvasToWidget(m_textPos);
    QFont scaledFont = effectiveFont();
    scaledFont.setPointSizeF(std::max(1.0, m_font.pointSizeF() * canvas.zoom()));
    painter.setFont(scaledFont);
    painter.setPen(QPen(canvas.document()->primaryColor()));
    painter.setOpacity(m_opacity / 100.0);

    // Render each line and append a caret to the last one.
    QFontMetricsF fm(scaledFont);
    const double lineH = fm.lineSpacing();
    QStringList lines = (m_text + "|").split('\n');
    double y = wPos.y();
    for (const QString &line : lines) {
        painter.drawText(QPointF(wPos.x() + alignOffset(line, fm), y), line);
        y += lineH;
    }
    painter.restore();
}

void TextTool::commitText(CanvasWidget &canvas) {
    if (m_text.isEmpty()) { m_editing = false; return; }

    auto *doc = canvas.document();
    auto *layer = doc->activeLayer();
    if (!layer || layer->isLocked()) { m_editing = false; return; }

    QImage before = layer->image().copy();

    const QFont drawFont = effectiveFont();
    QPainter painter(&layer->image());
    if (m_antialiased) painter.setRenderHint(QPainter::Antialiasing);
    clipToSelection(painter, doc);
    painter.setFont(drawFont);
    painter.setPen(doc->primaryColor());
    painter.setOpacity(m_opacity / 100.0);

    // drawText(point,...) ignores newlines, so lay out lines manually.
    QFontMetricsF fm(drawFont);
    const double lineH = fm.lineSpacing();
    double y = m_textPos.y();
    for (const QString &line : m_text.split('\n')) {
        painter.drawText(QPointF(m_textPos.x() + alignOffset(line, fm), y), line);
        y += lineH;
    }
    painter.end();

    doc->pushImageEdit(doc->activeLayerIndex(), before, "Text");
    m_editing = false;
    m_text.clear();
}
