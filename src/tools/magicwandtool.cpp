#include "magicwandtool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"

void MagicWandTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton) return;
    auto *doc = canvas.document();
    if (!doc) return;

    QPoint pos = canvasPos.toPoint();
    QImage image = doc->flattenVisible();
    if (!image.valid(pos)) return;

    SelectionMode mode = SelectionMode::Replace;
    if (event->modifiers() & Qt::ShiftModifier) mode = SelectionMode::Add;
    else if (event->modifiers() & Qt::ControlModifier) mode = SelectionMode::Subtract;

    if (m_global) {
        // Select all pixels of the same color globally
        QRgb targetColor = image.pixel(pos);
        QImage mask(image.size(), QImage::Format_Grayscale8);
        mask.fill(0);
        int tr = qRed(targetColor), tg = qGreen(targetColor), tb = qBlue(targetColor), ta = qAlpha(targetColor);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QRgb c = image.pixel(x, y);
                int dr = qRed(c) - tr, dg = qGreen(c) - tg, db = qBlue(c) - tb, da = qAlpha(c) - ta;
                int dist = static_cast<int>(std::sqrt(dr*dr + dg*dg + db*db + da*da));
                if (dist <= m_tolerance)
                    mask.scanLine(y)[x] = 255;
            }
        }
        // Apply mask as selection
        QPainterPath path;
        for (int y = 0; y < mask.height(); ++y) {
            const uchar *line = mask.constScanLine(y);
            const int w = mask.width();
            int start = -1;
            for (int x = 0; x < w; ++x) {
                if (line[x] > 0 && start == -1) start = x;
                else if (line[x] == 0 && start != -1) {
                    path.addRect(start, y, x - start, 1);
                    start = -1;
                }
            }
            if (start != -1) path.addRect(start, y, w - start, 1);
        }
        doc->selection().selectPath(path, mode);
    } else {
        doc->selection().selectByColor(image, pos, m_tolerance, mode);
    }

    emit doc->selectionChanged();
}

void MagicWandTool::mouseMoveEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}
void MagicWandTool::mouseReleaseEvent(const QPointF &, QMouseEvent *, CanvasWidget &) {}
