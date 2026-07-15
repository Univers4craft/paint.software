#include "shapetool.h"
#include "canvas/canvaswidget.h"
#include "core/document.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

void ShapeTool::mousePressEvent(const QPointF &canvasPos, QMouseEvent *event, CanvasWidget &canvas) {
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) return;
    auto *layer = canvas.document()->activeLayer();
    if (!layer || layer->isLocked()) return;

    m_drawing = true;
    m_startPos = canvasPos;
    m_currentPos = canvasPos;
    m_beforeImage = layer->image().copy();
    m_useRight = (event->button() == Qt::RightButton);
}

void ShapeTool::mouseMoveEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_currentPos = canvasPos;

    // Live preview: restore + draw
    auto *layer = canvas.document()->activeLayer();
    if (layer) {
        layer->setImage(m_beforeImage.copy());
        QPainter painter(&layer->image());
        if (m_antialiased) painter.setRenderHint(QPainter::Antialiasing);
        clipToSelection(painter, canvas.document());

        auto *doc = canvas.document();
        QColor primary = m_useRight ? doc->secondaryColor() : doc->primaryColor();
        QColor secondary = m_useRight ? doc->primaryColor() : doc->secondaryColor();

        QPen pen(primary, m_brushSize);
        painter.setPen((m_shapeFill == ShapeFill::Filled) ? Qt::NoPen : pen);
        painter.setBrush((m_shapeFill == ShapeFill::Outline) ? Qt::NoBrush : QBrush(secondary));
        painter.setOpacity(m_opacity / 100.0);

        QRectF rect(m_startPos, m_currentPos);
        drawShape(painter, rect.normalized());
    }
}

void ShapeTool::mouseReleaseEvent(const QPointF &canvasPos, QMouseEvent *, CanvasWidget &canvas) {
    if (!m_drawing) return;
    m_drawing = false;
    m_currentPos = canvasPos;
    canvas.document()->pushImageEdit(canvas.document()->activeLayerIndex(), m_beforeImage, "Shape");
}

void ShapeTool::drawOverlay(QPainter &, const CanvasWidget &) {}

void ShapeTool::drawShape(QPainter &painter, const QRectF &rect) {
    switch (m_shapeType) {
    case ShapeType::Rectangle:
        painter.drawRect(rect);
        break;
    case ShapeType::Ellipse:
        painter.drawEllipse(rect);
        break;
    case ShapeType::RoundedRectangle:
        painter.drawRoundedRect(rect, 10, 10);
        break;
    case ShapeType::Triangle: {
        QPainterPath path;
        path.moveTo(rect.center().x(), rect.top());
        path.lineTo(rect.bottomRight());
        path.lineTo(rect.bottomLeft());
        path.closeSubpath();
        painter.drawPath(path);
        break;
    }
    case ShapeType::Diamond: {
        QPainterPath path;
        path.moveTo(rect.center().x(), rect.top());
        path.lineTo(rect.right(), rect.center().y());
        path.lineTo(rect.center().x(), rect.bottom());
        path.lineTo(rect.left(), rect.center().y());
        path.closeSubpath();
        painter.drawPath(path);
        break;
    }
    case ShapeType::Pentagon: {
        QPainterPath path;
        double cx = rect.center().x(), cy = rect.center().y();
        double rx = rect.width() / 2, ry = rect.height() / 2;
        for (int i = 0; i < 5; ++i) {
            double angle = -M_PI / 2 + i * 2 * M_PI / 5;
            double x = cx + rx * cos(angle);
            double y = cy + ry * sin(angle);
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        path.closeSubpath();
        painter.drawPath(path);
        break;
    }
    case ShapeType::Hexagon: {
        QPainterPath path;
        double cx = rect.center().x(), cy = rect.center().y();
        double rx = rect.width() / 2, ry = rect.height() / 2;
        for (int i = 0; i < 6; ++i) {
            double angle = -M_PI / 2 + i * 2 * M_PI / 6;
            double x = cx + rx * cos(angle);
            double y = cy + ry * sin(angle);
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        path.closeSubpath();
        painter.drawPath(path);
        break;
    }
    case ShapeType::Star: {
        QPainterPath path;
        double cx = rect.center().x(), cy = rect.center().y();
        double rx = rect.width() / 2, ry = rect.height() / 2;
        for (int i = 0; i < 10; ++i) {
            double angle = -M_PI / 2 + i * M_PI / 5;
            double r = (i % 2 == 0) ? 1.0 : 0.4;
            double x = cx + rx * r * cos(angle);
            double y = cy + ry * r * sin(angle);
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        path.closeSubpath();
        painter.drawPath(path);
        break;
    }
    }
}
