#include "crystalizeeffect.h"
#include <cstdlib>
#include <cmath>
#include <vector>

QImage CrystalizeEffect::apply(const QImage &input) {
    QImage img = input.convertToFormat(QImage::Format_ARGB32);
    QImage result = img.copy();
    int w = img.width(), h = img.height();

    // Generate Voronoi cell centers
    int numCellsX = (w + m_cellSize - 1) / m_cellSize;
    int numCellsY = (h + m_cellSize - 1) / m_cellSize;

    struct Cell { int x, y; int rSum, gSum, bSum, aSum, count; };
    std::vector<Cell> cells;
    cells.reserve(numCellsX * numCellsY);

    srand(42);
    for (int cy = 0; cy < numCellsY; ++cy) {
        for (int cx = 0; cx < numCellsX; ++cx) {
            int px = cx * m_cellSize + rand() % m_cellSize;
            int py = cy * m_cellSize + rand() % m_cellSize;
            px = qBound(0, px, w - 1);
            py = qBound(0, py, h - 1);
            cells.push_back({px, py, 0, 0, 0, 0, 0});
        }
    }

    // Assign each pixel to nearest cell and accumulate color
    std::vector<int> assignments(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int bestCell = 0;
            double bestDist = 1e18;
            // Only check nearby cells for performance
            int gridX = x / m_cellSize, gridY = y / m_cellSize;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int ncx = gridX + dx, ncy = gridY + dy;
                    // Clamp to the grid so neighbour lookups never wrap onto the
                    // previous/next row (which caused seams at cell-column edges).
                    if (ncx < 0 || ncx >= numCellsX || ncy < 0 || ncy >= numCellsY) continue;
                    int gi = ncy * numCellsX + ncx;
                    double d = std::pow(x - cells[gi].x, 2) + std::pow(y - cells[gi].y, 2);
                    if (d < bestDist) { bestDist = d; bestCell = gi; }
                }
            }
            assignments[y * w + x] = bestCell;
            QRgb p = img.pixel(x, y);
            cells[bestCell].rSum += qRed(p);
            cells[bestCell].gSum += qGreen(p);
            cells[bestCell].bSum += qBlue(p);
            cells[bestCell].aSum += qAlpha(p);
            cells[bestCell].count++;
        }
    }

    // Fill each pixel with its cell's average color
    for (int y = 0; y < h; ++y) {
        QRgb *dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            auto &c = cells[assignments[y * w + x]];
            if (c.count > 0)
                dst[x] = qRgba(c.rSum / c.count, c.gSum / c.count, c.bSum / c.count, c.aSum / c.count);
        }
    }
    return result;
}
