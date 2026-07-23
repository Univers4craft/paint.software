#include "hatchpatterns.h"

#include <QImage>
#include <QPainter>
#include <QVector>
#include <array>

namespace Hatch {
namespace {

// A fill style: a display name plus an 8x8 (or 16x16) coverage tile where a set
// bit means "foreground". Solid is handled separately, so this list is the 53
// GDI+ hatch styles in enum order.
struct Pattern {
    const char *name;
    QImage mask;   // Format_Grayscale8, 255 = foreground pixel
};

// --- small tile builders -------------------------------------------------

QImage blank(int n) {
    QImage m(n, n, QImage::Format_Grayscale8);
    m.fill(0);
    return m;
}
inline void set(QImage &m, int x, int y) {
    const int n = m.width();
    m.scanLine(((y % n) + n) % n)[((x % n) + n) % n] = 255;
}

// Ordered-dither dot pattern at coverage `p` (0..1) — matches GDI+'s PercentNN.
QImage percent(double p) {
    static const int bayer[8][8] = {
        { 0,48,12,60, 3,51,15,63},{32,16,44,28,35,19,47,31},
        { 8,56, 4,52,11,59, 7,55},{40,24,36,20,43,27,39,23},
        { 2,50,14,62, 1,49,13,61},{34,18,46,30,33,17,45,29},
        {10,58, 6,54, 9,57, 5,53},{42,26,38,22,41,25,37,21}};
    QImage m = blank(8);
    const int thr = int(p * 64.0 + 0.5);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            if (bayer[y][x] < thr) m.scanLine(y)[x] = 255;
    return m;
}

// Diagonal lines. dir +1 = "/" (upward), -1 = "\" (downward). `step` is the gap
// between lines, `thick` the line thickness.
QImage diagonal(int dir, int step, int thick, int n = 8) {
    QImage m = blank(n);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const int d = (dir > 0) ? (x + y) : (x - y);
            if ((((d % step) + step) % step) < thick) m.scanLine(y)[x] = 255;
        }
    return m;
}

// Straight lines. vertical? spacing and thickness in pixels.
QImage lines(bool vertical, int step, int thick, int n = 8) {
    QImage m = blank(n);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const int c = vertical ? x : y;
            if ((c % step) < thick) m.scanLine(y)[x] = 255;
        }
    return m;
}

// From an 8-row ASCII bitmap ('#'/'X' = foreground).
QImage bits(std::array<const char *, 8> rows) {
    QImage m = blank(8);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8 && rows[y][x]; ++x)
            if (rows[y][x] != ' ' && rows[y][x] != '.') m.scanLine(y)[x] = 255;
    return m;
}

QImage cross(int step, int thick) {
    QImage m = lines(false, step, thick);
    QImage v = lines(true, step, thick);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            if (v.scanLine(y)[x]) m.scanLine(y)[x] = 255;
    return m;
}
QImage diagCross(int step, int thick) {
    QImage a = diagonal(+1, step, thick);
    QImage b = diagonal(-1, step, thick);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            if (b.scanLine(y)[x]) a.scanLine(y)[x] = 255;
    return a;
}

const QVector<Pattern> &patterns() {
    static const QVector<Pattern> p = [] {
        QVector<Pattern> v;
        auto add = [&](const char *n, QImage m) { v.push_back({n, std::move(m)}); };

        add("Horizontal",           lines(false, 8, 1));
        add("Vertical",             lines(true, 8, 1));
        add("Forward Diagonal",     diagonal(+1, 8, 1));
        add("Backward Diagonal",    diagonal(-1, 8, 1));
        add("Cross",                cross(8, 1));
        add("Diagonal Cross",       diagCross(8, 1));
        add("5%",                   percent(0.05));
        add("10%",                  percent(0.10));
        add("20%",                  percent(0.20));
        add("25%",                  percent(0.25));
        add("30%",                  percent(0.30));
        add("40%",                  percent(0.40));
        add("50%",                  percent(0.50));
        add("60%",                  percent(0.60));
        add("70%",                  percent(0.70));
        add("75%",                  percent(0.75));
        add("80%",                  percent(0.80));
        add("90%",                  percent(0.90));
        add("Light Downward Diagonal", diagonal(-1, 4, 1));
        add("Light Upward Diagonal",   diagonal(+1, 4, 1));
        add("Dark Downward Diagonal",  diagonal(-1, 4, 2));
        add("Dark Upward Diagonal",    diagonal(+1, 4, 2));
        add("Wide Downward Diagonal",  diagonal(-1, 8, 3));
        add("Wide Upward Diagonal",    diagonal(+1, 8, 3));
        add("Light Vertical",       lines(true, 4, 1));
        add("Light Horizontal",     lines(false, 4, 1));
        add("Narrow Vertical",      lines(true, 2, 1));
        add("Narrow Horizontal",    lines(false, 2, 1));
        add("Dark Vertical",        lines(true, 4, 2));
        add("Dark Horizontal",      lines(false, 4, 2));
        add("Dashed Downward Diagonal", bits({"#   #   ","  #   # "," #   #  ","        ","#   #   ","  #   # "," #   #  ","        "}));
        add("Dashed Upward Diagonal",   bits({"   #   #"," #   #  ","  #   # ","        ","   #   #"," #   #  ","  #   # ","        "}));
        add("Dashed Horizontal",    bits({"####    ","        ","    ####","        ","####    ","        ","    ####","        "}));
        add("Dashed Vertical",      bits({"#   #   ","#   #   ","#   #   ","        ","#   #   ","#   #   ","#   #   ","        "}));
        add("Small Confetti",       bits({"#    #  ","   #    "," #    # ","    #  #","#   #   ","  #    #","#  #  # ","   #  # "}));
        add("Large Confetti",       bits({"##   ## ","##    # ","   ##   ","  ##  ##","#    ## ","##   #  ","  #   ##"," ##  #  "}));
        add("ZigZag",               bits({"#      #"," #    # ","  #  #  ","   ##   ","#      #"," #    # ","  #  #  ","   ##   "}));
        add("Wave",                 bits({"        ","   ##   ","  #  #  "," #    # ","#      #","        ","        ","        "}));
        add("Diagonal Brick",       bits({"#       "," #      ","  #     ","   #    ","####### ","      # ","     #  ","    #   "}));
        add("Horizontal Brick",     bits({"#######.","......#.","......#.","#######.","#.....#.","#.....#.","#######.","......#."}));
        add("Weave",                bits({"#  #  # ","# #  # #"," # #  # ","  #  #  "," # #  # ","# #  # #","#  #  # ","    #   "}));
        add("Plaid",                bits({"# # # # "," # # # #","# # # # ","########","# # # # "," # # # #","# # # # ","        "}));
        add("Divot",                bits({"  #     ","   #    ","        ","        ","      # ","     #  ","        ","        "}));
        add("Dotted Grid",          bits({"#   #   ","        ","        ","        ","#   #   ","        ","        ","        "}));
        add("Dotted Diamond",       bits({"#   #   ","        ","  #   # ","        ","#   #   ","        ","  #   # ","        "}));
        add("Shingle",              bits({"#      #"," #    # ","  #  #  ","   ##   ","   #    ","   #    ","   #    ","   #    "}));
        add("Trellis",              bits({"########","        ","## ## ##","        ","########","        ","## ## ##","        "}));
        add("Sphere",               bits({" ###### ","#      #","#  ##  #","#  ##  #","#      #"," ###### ","        ","        "}));
        add("Small Grid",           cross(4, 1));
        add("Small Checker Board",  bits({"##  ##  ","##  ##  ","  ##  ##","  ##  ##","##  ##  ","##  ##  ","  ##  ##","  ##  ##"}));
        add("Large Checker Board",  bits({"####    ","####    ","####    ","####    ","    ####","    ####","    ####","    ####"}));
        add("Outlined Diamond",     bits({"   #    ","  # #   "," #   #  ","#     # "," #   #  ","  # #   ","   #    ","        "}));
        add("Solid Diamond",        bits({"...#....","..###...",".#####..","#######.",".#####..","..###...","...#....","........"}));

        return v;
    }();
    return p;
}

}  // namespace

int count() { return patterns().size() + 1; }   // +1 for Solid at index 0

QString name(int index) {
    if (index <= 0) return QStringLiteral("Solid Color");
    if (index - 1 < patterns().size()) return QString::fromLatin1(patterns()[index - 1].name);
    return QStringLiteral("Solid Color");
}

QBrush brush(int index, const QColor &fg, const QColor &bg) {
    if (index <= 0 || index - 1 >= patterns().size())
        return QBrush(fg);

    const QImage &mask = patterns()[index - 1].mask;
    const int n = mask.width();
    QImage tile(n, n, QImage::Format_ARGB32_Premultiplied);
    QPainter pr(&tile);
    pr.setCompositionMode(QPainter::CompositionMode_Source);
    pr.fillRect(tile.rect(), bg);          // background colour (may be transparent)
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
            if (mask.constScanLine(y)[x])
                tile.setPixelColor(x, y, fg);   // foreground pattern
    pr.end();
    return QBrush(tile);
}

}  // namespace Hatch
