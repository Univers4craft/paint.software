#include "selection.h"
#include <QPainter>
#include <QQueue>
#include <cmath>

Selection::Selection() : m_mask(1, 1, QImage::Format_Grayscale8), m_empty(true) {
    m_mask.fill(0);
}

Selection::Selection(int width, int height)
    : m_mask(width, height, QImage::Format_Grayscale8), m_empty(true)
{
    m_mask.fill(0);
}

QRegion Selection::region() const {
    if (m_empty) return QRegion();
    QRegion reg;
    const int w = m_mask.width();
    for (int y = 0; y < m_mask.height(); ++y) {
        const uchar *line = m_mask.constScanLine(y);
        int start = -1;
        for (int x = 0; x < w; ++x) {
            bool sel = line[x] > 127;
            if (sel && start == -1) {
                start = x;
            } else if (!sel && start != -1) {
                reg += QRect(start, y, x - start, 1);
                start = -1;
            }
        }
        // Flush a run that reaches (or starts at) the last column.
        if (start != -1)
            reg += QRect(start, y, w - start, 1);
    }
    return reg;
}

QRect Selection::boundingRect() const {
    if (m_empty) return QRect();
    int minX = m_mask.width(), minY = m_mask.height(), maxX = 0, maxY = 0;
    for (int y = 0; y < m_mask.height(); ++y) {
        const uchar *line = m_mask.constScanLine(y);
        for (int x = 0; x < m_mask.width(); ++x) {
            if (line[x] > 0) {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }
    if (maxX < minX) return QRect();
    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

void Selection::clear() {
    m_mask.fill(0);
    m_empty = true;
}

void Selection::selectAll() {
    m_mask.fill(255);
    m_empty = false;
}

void Selection::invert() {
    for (int y = 0; y < m_mask.height(); ++y) {
        uchar *line = m_mask.scanLine(y);
        for (int x = 0; x < m_mask.width(); ++x) {
            line[x] = 255 - line[x];
        }
    }
    updateEmpty();
}

void Selection::selectRect(const QRect &rect, SelectionMode mode) {
    QImage newMask(m_mask.size(), QImage::Format_Grayscale8);
    newMask.fill(0);
    QPainter p(&newMask);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRect(rect);
    p.end();
    applyMode(newMask, mode);
}

void Selection::selectEllipse(const QRect &rect, SelectionMode mode) {
    QImage newMask(m_mask.size(), QImage::Format_Grayscale8);
    newMask.fill(0);
    QPainter p(&newMask);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(rect);
    p.end();
    applyMode(newMask, mode);
}

void Selection::selectPath(const QPainterPath &path, SelectionMode mode) {
    QImage newMask(m_mask.size(), QImage::Format_Grayscale8);
    newMask.fill(0);
    QPainter p(&newMask);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawPath(path);
    p.end();
    applyMode(newMask, mode);
}

void Selection::selectByColor(const QImage &image, const QPoint &pos, int tolerance, SelectionMode mode) {
    if (!image.valid(pos)) return;

    QImage newMask(m_mask.size(), QImage::Format_Grayscale8);
    newMask.fill(0);

    QRgb targetColor = image.pixel(pos);
    int tr = qRed(targetColor), tg = qGreen(targetColor), tb = qBlue(targetColor), ta = qAlpha(targetColor);

    QVector<QVector<bool>> visited(image.height(), QVector<bool>(image.width(), false));
    QQueue<QPoint> queue;
    queue.enqueue(pos);
    visited[pos.y()][pos.x()] = true;

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();
        QRgb c = image.pixel(p);
        int dr = qRed(c) - tr, dg = qGreen(c) - tg, db = qBlue(c) - tb, da = qAlpha(c) - ta;
        int dist = std::sqrt(dr*dr + dg*dg + db*db + da*da);

        if (dist <= tolerance) {
            newMask.scanLine(p.y())[p.x()] = 255;
            const QPoint neighbors[] = {{p.x()-1,p.y()},{p.x()+1,p.y()},{p.x(),p.y()-1},{p.x(),p.y()+1}};
            for (const QPoint &n : neighbors) {
                if (n.x() >= 0 && n.x() < image.width() && n.y() >= 0 && n.y() < image.height() && !visited[n.y()][n.x()]) {
                    visited[n.y()][n.x()] = true;
                    queue.enqueue(n);
                }
            }
        }
    }
    applyMode(newMask, mode);
}

void Selection::setMaskImage(const QImage &mask) {
    if (mask.format() == QImage::Format_Grayscale8) {
        m_mask = mask.copy();
    } else {
        m_mask = mask.convertToFormat(QImage::Format_Grayscale8);
    }
    updateEmpty();
}

void Selection::feather(int radius) {
    if (m_empty || radius <= 0) return;
    // Gaussian-like blur on mask. Sample with clamped bounds so the outermost
    // 1px border is feathered too (it was previously left hard).
    const int W = m_mask.width(), H = m_mask.height();
    QImage temp = m_mask.copy();
    for (int pass = 0; pass < radius; ++pass) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int sum = 0, n = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    int ny = y + dy;
                    if (ny < 0 || ny >= H) continue;
                    const uchar *tl = temp.constScanLine(ny);
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        if (nx < 0 || nx >= W) continue;
                        sum += tl[nx];
                        ++n;
                    }
                }
                m_mask.scanLine(y)[x] = static_cast<uchar>(sum / n);
            }
        }
        temp = m_mask.copy();
    }
    updateEmpty();
}

void Selection::expand(int pixels) {
    for (int i = 0; i < pixels; ++i) {
        QImage temp = m_mask.copy();
        for (int y = 0; y < m_mask.height(); ++y) {
            for (int x = 0; x < m_mask.width(); ++x) {
                uchar maxVal = temp.constScanLine(y)[x];
                if (y > 0) maxVal = std::max(maxVal, temp.constScanLine(y-1)[x]);
                if (y < m_mask.height()-1) maxVal = std::max(maxVal, temp.constScanLine(y+1)[x]);
                if (x > 0) maxVal = std::max(maxVal, temp.constScanLine(y)[x-1]);
                if (x < m_mask.width()-1) maxVal = std::max(maxVal, temp.constScanLine(y)[x+1]);
                m_mask.scanLine(y)[x] = maxVal;
            }
        }
    }
    updateEmpty();
}

void Selection::contract(int pixels) {
    invert();
    expand(pixels);
    invert();
}

void Selection::translate(const QPoint &delta) {
    if (m_empty) return;
    QImage newMask(m_mask.size(), QImage::Format_Grayscale8);
    newMask.fill(0);
    QPainter p(&newMask);
    p.drawImage(delta, m_mask);
    p.end();
    m_mask = newMask;
    updateEmpty();
}

void Selection::resize(int width, int height) {
    QImage newMask(width, height, QImage::Format_Grayscale8);
    newMask.fill(0);
    QPainter p(&newMask);
    p.drawImage(0, 0, m_mask);
    m_mask = newMask;
    updateEmpty();
}

bool Selection::isSelected(int x, int y) const {
    if (m_empty) return true;
    if (x < 0 || x >= m_mask.width() || y < 0 || y >= m_mask.height()) return false;
    return m_mask.constScanLine(y)[x] > 127;
}

quint8 Selection::selectionValue(int x, int y) const {
    if (m_empty) return 255;
    if (x < 0 || x >= m_mask.width() || y < 0 || y >= m_mask.height()) return 0;
    return m_mask.constScanLine(y)[x];
}

void Selection::applyToImage(QImage &image) const {
    applyToImage(image, QPoint());
}

void Selection::applyToImage(QImage &image, const QPoint &imageOffset) const {
    if (m_empty) return;
    QImage maskImage = maskForImage(image.size(), imageOffset);
    for (int y = 0; y < image.height(); ++y) {
        QRgb *pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
        const uchar *maskLine = maskImage.constScanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            const int amount = maskLine[x];
            pixels[x] = qRgba(
                qRed(pixels[x]) * amount / 255,
                qGreen(pixels[x]) * amount / 255,
                qBlue(pixels[x]) * amount / 255,
                qAlpha(pixels[x]) * amount / 255);
        }
    }
}

void Selection::eraseFromImage(QImage &image) const {
    eraseFromImage(image, QPoint());
}

void Selection::eraseFromImage(QImage &image, const QPoint &imageOffset) const {
    if (m_empty) return;
    QImage maskImage = maskForImage(image.size(), imageOffset);
    for (int y = 0; y < image.height(); ++y) {
        QRgb *pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
        const uchar *maskLine = maskImage.constScanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            const int keep = 255 - maskLine[x];
            pixels[x] = qRgba(
                qRed(pixels[x]) * keep / 255,
                qGreen(pixels[x]) * keep / 255,
                qBlue(pixels[x]) * keep / 255,
                qAlpha(pixels[x]) * keep / 255);
        }
    }
}

QImage Selection::maskForImage(const QSize &size, const QPoint &imageOffset) const {
    QImage maskImage(size, QImage::Format_Grayscale8);
    maskImage.fill(0);
    if (m_empty) return maskImage;

    QPainter painter(&maskImage);
    painter.drawImage(-imageOffset, m_mask);
    painter.end();
    return maskImage;
}

QImage Selection::getMaskedImage(const QImage &source) const {
    return getMaskedImage(source, QPoint());
}

QImage Selection::getMaskedImage(const QImage &source, const QPoint &imageOffset) const {
    QImage result = source.copy();
    applyToImage(result, imageOffset);
    return result;
}

void Selection::applyMode(const QImage &newMask, SelectionMode mode) {
    switch (mode) {
    case SelectionMode::Replace:
        m_mask = newMask;
        break;
    case SelectionMode::Add:
        for (int y = 0; y < m_mask.height(); ++y) {
            uchar *dst = m_mask.scanLine(y);
            const uchar *src = newMask.constScanLine(y);
            for (int x = 0; x < m_mask.width(); ++x)
                dst[x] = std::max(dst[x], src[x]);
        }
        break;
    case SelectionMode::Subtract:
        for (int y = 0; y < m_mask.height(); ++y) {
            uchar *dst = m_mask.scanLine(y);
            const uchar *src = newMask.constScanLine(y);
            for (int x = 0; x < m_mask.width(); ++x)
                dst[x] = std::max(0, dst[x] - src[x]);
        }
        break;
    case SelectionMode::Intersect:
        for (int y = 0; y < m_mask.height(); ++y) {
            uchar *dst = m_mask.scanLine(y);
            const uchar *src = newMask.constScanLine(y);
            for (int x = 0; x < m_mask.width(); ++x)
                dst[x] = std::min(dst[x], src[x]);
        }
        break;
    case SelectionMode::Invert:
        // XOR: a pixel ends up selected where exactly one of the two regions
        // covered it. Overlaps cancel out.
        for (int y = 0; y < m_mask.height(); ++y) {
            uchar *dst = m_mask.scanLine(y);
            const uchar *src = newMask.constScanLine(y);
            for (int x = 0; x < m_mask.width(); ++x)
                dst[x] = std::max(dst[x], src[x]) - std::min(dst[x], src[x]);
        }
        break;
    }
    updateEmpty();
}

void Selection::updateEmpty() {
    m_empty = true;
    for (int y = 0; y < m_mask.height() && m_empty; ++y) {
        const uchar *line = m_mask.constScanLine(y);
        for (int x = 0; x < m_mask.width(); ++x) {
            if (line[x] > 0) { m_empty = false; break; }
        }
    }
}
