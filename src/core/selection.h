#pragma once

#include <QImage>
#include <QRegion>
#include <QPainterPath>
#include <QPoint>

enum class SelectionMode {
    Replace,
    Add,
    Subtract,
    Intersect
};

class Selection {
public:
    Selection();
    Selection(int width, int height);

    bool isEmpty() const { return m_empty; }
    bool hasSelection() const { return !m_empty; }

    const QImage &mask() const { return m_mask; }
    QRegion region() const;
    QRect boundingRect() const;

    void clear();
    void selectAll();
    void invert();
    void selectRect(const QRect &rect, SelectionMode mode = SelectionMode::Replace);
    void selectEllipse(const QRect &rect, SelectionMode mode = SelectionMode::Replace);
    void selectPath(const QPainterPath &path, SelectionMode mode = SelectionMode::Replace);
    void selectByColor(const QImage &image, const QPoint &pos, int tolerance, SelectionMode mode = SelectionMode::Replace);
    void setMaskImage(const QImage &mask);

    void feather(int radius);
    void expand(int pixels);
    void contract(int pixels);
    void translate(const QPoint &delta);

    void resize(int width, int height);

    bool isSelected(int x, int y) const;
    quint8 selectionValue(int x, int y) const;

    void applyToImage(QImage &image) const;
    void applyToImage(QImage &image, const QPoint &imageOffset) const;
    void eraseFromImage(QImage &image) const;
    void eraseFromImage(QImage &image, const QPoint &imageOffset) const;
    QImage maskForImage(const QSize &size, const QPoint &imageOffset = QPoint()) const;
    QImage getMaskedImage(const QImage &source) const;
    QImage getMaskedImage(const QImage &source, const QPoint &imageOffset) const;

private:
    void applyMode(const QImage &newMask, SelectionMode mode);
    void updateEmpty();

    QImage m_mask;
    bool m_empty = true;
};
