// Comprehensive headless functional test for PaintDali.
// Exercises the real code paths: tools, effects, adjustments, layers,
// selection, undo/redo, save/load, i18n and theming.
#include <QApplication>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>
#include <QElapsedTimer>
#include <QThread>
#include <QDir>
#include <QFile>
#include <cstdio>
#include <cstring>
#include <functional>
#include <climits>
#include <tuple>

#include "core/document.h"
#include "core/layer.h"
#include "core/selection.h"
#include "core/history.h"
#include "core/adjustmentpreview.h"
#include "core/pdnformat.h"
#include "canvas/canvaswidget.h"
#include "i18n.h"
#include "theme.h"

#include "tools/brushtool.h"
#include "tools/penciltool.h"
#include "tools/filltool.h"
#include "tools/erasertool.h"
#include "tools/linetool.h"
#include "tools/shapetool.h"
#include "tools/gradienttool.h"
#include "tools/recolortool.h"
#include "tools/clonestamptool.h"
#include "tools/selectiontool.h"
#include "tools/magicwandtool.h"
#include "tools/movetool.h"
#include "tools/moveselectiontool.h"
#include "tools/texttool.h"
#include "tools/colorpickertool.h"
#include "tools/lassotool.h"
#include "panels/colorspanel.h"
#include "panels/tooloptionspanel.h"
#include "core/hatchpatterns.h"
#include <QImageWriter>
#include <QImageReader>
#include <QTemporaryFile>
#include <QDataStream>
#include <QTemporaryDir>
#include <QFileInfo>
#include "dialogs/resizedialog.h"
#include "dialogs/previewdialog.h"
#include "dialogs/huesaturationdialog.h"
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include "dialogs/saveconfigdialog.h"
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QMainWindow>
#include <QKeyEvent>

#include "effects/blureffect.h"
#include "effects/sharpeneffect.h"
#include "effects/noiseeffect.h"
#include "effects/embosseffect.h"
#include "effects/edgedetecteffect.h"
#include "effects/oilpainteffect.h"
#include "effects/pixelateeffect.h"
#include "effects/motionblureffect.h"
#include "effects/radialblureffect.h"
#include "effects/zoomblureffect.h"
#include "effects/surfaceblureffect.h"
#include "effects/unfocuseffect.h"
#include "effects/fragmenteffect.h"
#include "effects/bulgeeffect.h"
#include "effects/twisteffect.h"
#include "effects/frostedglasseffect.h"
#include "effects/crystalizeeffect.h"
#include "effects/tileeffect.h"
#include "effects/dentseffect.h"
#include "effects/polarinversioneffect.h"
#include "effects/medianeffect.h"
#include "effects/reducenoiseeffect.h"
#include "effects/quantizeeffect.h"
#include "effects/gloweffect.h"
#include "effects/redeyeremoveeffect.h"
#include "effects/softenportraiteffect.h"
#include "effects/vignetteeffect.h"
#include "effects/turbulenceeffect.h"
#include "effects/reliefeffect.h"
#include "effects/outlineeffect.h"
#include "effects/morphologyeffect.h"

#include "adjustments/brightnesscontrast.h"
#include "adjustments/huesaturation.h"
#include "adjustments/levels.h"
#include "adjustments/curves.h"
#include "adjustments/invertcolors.h"
#include "adjustments/sepia.h"
#include "adjustments/posterize.h"
#include "adjustments/threshold.h"
#include "adjustments/desaturate.h"
#include "adjustments/colorbalance.h"
#include "adjustments/exposure.h"
#include "adjustments/highlightsshadows.h"
#include "adjustments/temperaturetint.h"

#include "plugins/plugin_api.h"
#include "plugins/plugineffect.h"
#include "plugins/pluginmanager.h"
#include <QLibrary>
#include <QCoreApplication>

static int g_pass = 0, g_fail = 0;
static const char *g_section = "";
#define SECTION(s) do { g_section = s; printf("\n== %s ==\n", s); } while(0)
#define CHECK(cond, msg) do { \
    if (cond) { ++g_pass; /* printf("  ok  : %s\n", msg); */ } \
    else { ++g_fail; printf("  FAIL [%s]: %s\n", g_section, msg); } \
} while(0)

// ---- helpers ----
static QImage makeImage(int w, int h, QColor fill) {
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(fill);
    return img;
}

static bool imagesDiffer(const QImage &a, const QImage &b) {
    if (a.size() != b.size()) return true;
    return a != b;
}

static int countNonBackground(const QImage &img, QColor bg) {
    int n = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixelColor(x, y) != bg) ++n;
    return n;
}

// Synthesize a full press-drag-release on a tool in document coordinates.
static QMouseEvent pressEv(QPointF p, Qt::MouseButton b = Qt::LeftButton) {
    return QMouseEvent(QEvent::MouseButtonPress, p, p, b, b, Qt::NoModifier);
}
static QMouseEvent moveEv(QPointF p, Qt::MouseButton b = Qt::LeftButton) {
    return QMouseEvent(QEvent::MouseMove, p, p, Qt::NoButton, b, Qt::NoModifier);
}
static QMouseEvent releaseEv(QPointF p, Qt::MouseButton b = Qt::LeftButton) {
    return QMouseEvent(QEvent::MouseButtonRelease, p, p, b, Qt::NoButton, Qt::NoModifier);
}

// A plugin-style process function (plain C signature): blend each RGB channel
// toward its inverse by values[0] percent. Used to test the plugin wrapper.
static void testInvertProcess(unsigned char *rgba, int w, int h,
                              const int *values, int valueCount, void *userData) {
    (void)userData;
    int amount = (valueCount > 0) ? values[0] : 100;
    for (long i = 0; i < (long)w * h; ++i) {
        unsigned char *p = rgba + i * 4;
        for (int c = 0; c < 3; ++c) {
            int inv = 255 - p[c];
            p[c] = (unsigned char)((p[c] * (100 - amount) + inv * amount) / 100);
        }
    }
}

static void strokeTool(Tool *tool, CanvasWidget &canvas, QPointF a, QPointF b) {
    QMouseEvent e1 = pressEv(a);   tool->mousePressEvent(a, &e1, canvas);
    QMouseEvent e2 = moveEv((a+b)/2); tool->mouseMoveEvent((a+b)/2, &e2, canvas);
    QMouseEvent e3 = moveEv(b);     tool->mouseMoveEvent(b, &e3, canvas);
    QMouseEvent e4 = releaseEv(b);  tool->mouseReleaseEvent(b, &e4, canvas);
}

// An asymmetric retained bitmap with visible and off-canvas markers.
static QImage retainedPattern() {
    QImage image = makeImage(840, 630, Qt::transparent);
    QPainter p(&image);
    p.fillRect(25, 25, 15, 15, Qt::yellow);
    p.fillRect(345, 290, 15, 15, Qt::cyan);
    p.fillRect(80, 90, 35, 45, Qt::red);
    p.fillRect(200, 220, 40, 20, Qt::green);
    p.fillRect(805, 595, 25, 25, Qt::blue);
    return image;
}

// Characterization: setImage already normalizes both resize branches, even
// when a caller bypasses it through the mutable bitmap accessor.
static void resizeStraightAlphaCharacterization() {
    SECTION("Resize normalizes straight-alpha input in ordinary and retained layers");
    for (bool retained : {false, true}) {
        Document doc(8,6);
        QImage straight(retained ? QSize(12,10) : doc.size(), QImage::Format_ARGB32);
        straight.fill(Qt::transparent);
        straight.setPixelColor(3,3,QColor(220,80,30,117));
        straight.setPixelColor(5,4,QColor(20,140,230,201));
        auto *layer = doc.activeLayer();
        layer->image() = straight;
        layer->setRetainsOffCanvasPixels(retained);
        layer->setOffset(retained ? QPoint(-2,-1) : QPoint());
        // Scale the finite straight-alpha source independently; comparing a
        // pre-composited image would introduce a different rounding order.
        const QImage expected = straight.copy(QRect(-layer->offset(),doc.size())).scaled(
            13,11,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32_Premultiplied);
        doc.resize(13,11);
        CHECK(doc.activeLayer()->image().format() == QImage::Format_ARGB32_Premultiplied,
              "resize normalizes even directly injected straight-alpha storage");
        CHECK(doc.flatten() == expected, "straight-alpha resize preserves visible alpha and colours");
    }
}

static void retainedCropRegression() {
    SECTION("Retained crop uses document coordinates including selection edges");
    for (QPoint offset : {QPoint(50, 40), QPoint(-50, -40)}) {
        for (QRect requested : {QRect(70, 60, 240, 270), QRect(-10, -20, 160, 180),
                                QRect(730, 530, 100, 100)}) {
            Document doc(800, 600);
            doc.activeLayer()->clear(Qt::transparent);
            const QImage image = retainedPattern();
            doc.pasteImage(image, "retained", false);
            doc.activeLayer()->setOffset(offset);
            doc.selection().selectRect(requested);
            const QImage mask = doc.selection().mask();
            const QImage before = doc.flatten();
            const QRect crop = requested.intersected(before.rect());
            doc.cropTo(requested);
            const QImage after = doc.flatten();
            CHECK(after == before.copy(crop), "crop preserves exactly the visible document-space pixels");
            CHECK(doc.activeLayer()->offset().isNull(), "destructive crop rebases the layer origin");
            CHECK(doc.activeLayer()->image().size() == crop.size(), "crop discards pixels outside its bounds");
            CHECK(!doc.activeLayer()->retainsOffCanvasPixels(), "destructive crop clears retained movement state");
            CHECK(!doc.selection().hasSelection(), "crop clears selection");
            doc.history().undo();
            CHECK(doc.size() == QSize(800,600) && doc.activeLayerIndex() == 1,
                  "crop undo restores size and active layer");
            CHECK(doc.activeLayer()->image() == image && doc.activeLayer()->offset() == offset,
                  "crop undo restores the entire original retained bitmap and offset");
            CHECK(doc.selection().mask() == mask, "crop undo restores selection edges");
            CHECK(doc.activeLayer()->retainsOffCanvasPixels(), "crop undo restores retained state");
            doc.history().redo();
            CHECK(doc.flatten() == after && doc.activeLayer()->offset().isNull(), "crop redo restores pixels and origin");
            CHECK(!doc.activeLayer()->retainsOffCanvasPixels(), "crop redo clears retained state again");
            QTemporaryDir directory;
            const QString path = directory.path()+"/cropped.psw";
            CHECK(doc.saveNative(path), "save destructively cropped document");
            QFile file(path);
            CHECK(file.open(QIODevice::ReadOnly), "open cropped native file");
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_6_0);
            quint32 magic; qint32 version;
            stream >> magic >> version;
            CHECK(version == 1, "crop with no retained layers writes legacy v1");
            for (bool keepArtwork : {false,true}) {
                doc.mergeLayerDown(1,keepArtwork);
                CHECK(!doc.activeLayer()->retainsOffCanvasPixels(), "merge does not inherit stale crop state");
                doc.history().undo();
            }
            const QImage cropped = doc.activeLayer()->image();
            QImage moved(cropped.size(),QImage::Format_ARGB32_Premultiplied);
            moved.fill(Qt::transparent);
            { QPainter painter(&moved); painter.drawImage(3,2,cropped); }
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            MoveTool move;
            strokeTool(&move,canvas,QPointF(20,20),QPointF(23,22));
            CHECK(doc.activeLayer()->offset().isNull() && doc.activeLayer()->image() == moved,
                  "cropped layer returns to ordinary pixel-baked Move");
        }
    }
}

static void retainedExpansionRegression() {
    SECTION("Expansion makes negatively positioned retained layers editable at new edges");
    for (QPoint origin : {QPoint(-50,-40),QPoint(-50,20),QPoint(20,-40),QPoint(20,30),QPoint()}) {
        for (QSize target : {QSize(900,600),QSize(800,700),QSize(900,700)}) {
            Document doc(800,600);
            doc.activeLayer()->clear(Qt::transparent);
            const QImage original = retainedPattern();
            doc.pasteImage(original,"Keep",false);
            doc.activeLayer()->setOffset(origin);
            doc.selection().selectRect(QRect(3,4,10,12));
            const QImage mask = doc.selection().mask();
            const QImage before = doc.flatten();
            QImage pasted = makeImage(target.width(),target.height(),Qt::transparent);
            doc.pasteImage(pasted,"Expand",true);
            doc.setActiveLayer(1);
            CHECK(doc.size() == target, "Expand uses componentwise maximum canvas dimensions");
            CHECK(doc.activeLayer()->offset() == origin, "Expand preserves exact retained origin");
            CHECK(doc.activeLayer()->image().copy(original.rect()) == original, "Expand never resamples or loses existing bitmap");
            CHECK(doc.flatten().copy(before.rect()) == before, "Expand preserves old visible artwork");
            const QImage padded = doc.activeLayer()->image();
            doc.history().undo();
            CHECK(doc.size() == QSize(800,600) && doc.activeLayer()->image() == original &&
                  doc.activeLayer()->offset() == origin && doc.selection().mask() == mask,
                  "Expand undo restores bitmap, origin, selection and dimensions atomically");
            doc.history().redo();
            doc.setActiveLayer(1);
            CHECK(doc.activeLayer()->image() == padded && doc.activeLayer()->offset() == origin &&
                  !doc.selection().hasSelection(), "Expand redo restores geometry and cleared selection");
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            doc.setPrimaryColor(Qt::magenta);
            PencilTool pencil;
            pencil.setBrushSize(1);
            const QPoint edge(target.width()-1,target.height()-1);
            strokeTool(&pencil,canvas,edge,edge);
            CHECK(doc.flatten().pixelColor(edge) == QColor(Qt::magenta),
                  "actual Pencil edit reaches the freshly expanded right/bottom canvas edge");
        }
    }
}

static void retainedOrthogonalRegression() {
    SECTION("Retained rotations and flips transform document-space origins");
    for (QPoint offset : {QPoint(50, 40), QPoint(-50, -40)}) {
        for (int operation = 0; operation < 5; ++operation) {
            Document doc(800, 600);
            doc.activeLayer()->clear(Qt::transparent);
            const QImage image = retainedPattern();
            doc.pasteImage(image, "retained", false);
            doc.activeLayer()->setOffset(offset);
            doc.selection().selectRect(QRect(10,20,50,60));
            const QImage mask = doc.selection().mask();
            const QImage before = doc.flatten();
            QImage expected, expectedBuffer;
            QPoint expectedOffset;
            if (operation < 3) {
                const int turns = operation + 1;
                QTransform transform;
                transform.rotate(turns * 90);
                expected = before.transformed(transform);
                expectedBuffer = image.transformed(transform);
                if (turns == 1) expectedOffset = QPoint(600-offset.y()-image.height(), offset.x());
                if (turns == 2) expectedOffset = QPoint(800-offset.x()-image.width(), 600-offset.y()-image.height());
                if (turns == 3) expectedOffset = QPoint(offset.y(), 800-offset.x()-image.width());
                doc.rotate(turns == 3 ? -1 : turns, "rotate");
            } else {
                const bool horizontal = operation == 3;
                const QTransform reflection = QTransform::fromScale(horizontal ? -1 : 1, horizontal ? 1 : -1);
                expected = before.transformed(reflection);
                expectedBuffer = image.transformed(reflection);
                expectedOffset = horizontal ? QPoint(800-offset.x()-image.width(), offset.y())
                                            : QPoint(offset.x(), 600-offset.y()-image.height());
                doc.flip(horizontal, "flip");
            }
            CHECK(doc.flatten() == expected, "orthogonal operation preserves exact visible composition");
            CHECK(doc.activeLayer()->image() == expectedBuffer, "orthogonal operation retains all off-canvas pixels");
            CHECK(doc.activeLayer()->offset() == expectedOffset, "orthogonal origin uses canvas rather than bitmap bounds");
            CHECK(!doc.selection().hasSelection(), "orthogonal operation clears selection");
            doc.history().undo();
            CHECK(doc.activeLayer()->image() == image && doc.activeLayer()->offset() == offset,
                  "orthogonal undo restores original buffer and origin");
            CHECK(doc.size() == before.size() && doc.selection().mask() == mask, "orthogonal undo restores canvas and selection");
            doc.history().redo();
            CHECK(doc.flatten() == expected && doc.activeLayer()->image() == expectedBuffer &&
                  doc.activeLayer()->offset() == expectedOffset, "orthogonal redo restores complete transformed state");
        }
    }
}

static void retainedResizeRegression() {
    SECTION("Retained image resize uses the document scale and sampling grid");
    for (QPoint offset : {QPoint(50, 40), QPoint(-50, -40), QPoint(51, -39)}) {
        for (QSize target : {QSize(400,300), QSize(1200,900), QSize(413,307)}) {
            Document doc(800,600);
            doc.activeLayer()->clear(Qt::transparent);
            QImage image = retainedPattern();
            // Semi-transparent detail and a marker crossing the visible canvas edge
            // distinguish resampling phase from mere bounding-box correctness.
            { QPainter p(&image); p.fillRect(795-offset.x(), 575-offset.y(), 15,20, QColor(90,130,210,123)); }
            doc.pasteImage(image, "retained", false);
            doc.activeLayer()->setOffset(offset);
            doc.selection().selectRect(QRect(30,40,50,60));
            const QImage mask = doc.selection().mask();
            const QImage expected = doc.flatten().scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            doc.resize(target.width(), target.height());
            const QImage after = doc.activeLayer()->image();
            const QPoint afterOffset = doc.activeLayer()->offset();
            CHECK(doc.flatten() == expected, "resize preserves exact visible canvas resampling, including edge alpha");
            CHECK(doc.activeLayer()->image().format() == QImage::Format_ARGB32_Premultiplied,
                  "resize keeps premultiplied storage");
            const QPoint hidden(qRound((offset.x()+817)*target.width()/800.0),
                                qRound((offset.y()+607)*target.height()/600.0));
            CHECK(doc.activeLayer()->image().rect().contains(hidden-afterOffset) &&
                  doc.activeLayer()->image().pixelColor(hidden-afterOffset) == QColor(Qt::blue),
                  "resize retains scaled off-canvas artwork rather than squeezing it into the canvas");
            doc.history().undo();
            CHECK(doc.activeLayer()->image() == image && doc.activeLayer()->offset() == offset &&
                  doc.size() == QSize(800,600), "resize undo restores original geometry and all pixels");
            CHECK(doc.selection().mask() == mask, "resize undo restores selection");
            doc.history().redo();
            CHECK(doc.flatten() == expected && doc.activeLayer()->image() == after &&
                  doc.activeLayer()->offset() == afterOffset, "resize redo restores sampled retained geometry");
        }
    }
}

static void pasteExpansionBoundsRegression() {
    SECTION("Expansion rejects unrepresentable local bitmap extents before mutation");
    for (QPoint origin : {QPoint(INT_MIN,0),QPoint(0,INT_MIN),QPoint(INT_MIN,INT_MIN)}) {
        Document doc(4,3);
        const QImage image = makeImage(6,5,Qt::red);
        doc.pasteImage(image,"Keep",false);
        doc.activeLayer()->setOffset(origin);
        doc.setModified(false);
        const QSize size = doc.size();
        const int count = doc.layerCount(), active = doc.activeLayerIndex();
        const int history = doc.history().currentIndex();
        const QImage mask = doc.selection().mask();
        const QImage pasted = makeImage(7,6,Qt::transparent);
        CHECK(doc.pasteImage(pasted,"Expand",true) == -1, "unrepresentable expansion is rejected");
        CHECK(doc.size() == size && doc.layerCount() == count && doc.activeLayerIndex() == active,
              "rejected expansion leaves document geometry and layer stack unchanged");
        CHECK(doc.history().currentIndex() == history && !doc.isModified() && doc.selection().mask() == mask,
              "rejected expansion leaves history, modified state and selection unchanged");
        CHECK(doc.layerAt(active)->image() == image && doc.layerAt(active)->offset() == origin,
              "rejected expansion preserves original bitmap and origin");
    }
}

static void resizeStampCharacterization() {
    SECTION("Resize Source stamp replaces only the finite canvas, never off-canvas pixels");
    for (QPoint origin : {QPoint(-2,-1),QPoint(2,1),QPoint(1,-2)}) {
        for (QSize target : {QSize(13,11),QSize(4,3)}) {
            Document doc(8,6);
            QImage image = makeImage(12,10,Qt::transparent);
            for (int y=0;y<image.height();++y)
                for (int x=0;x<image.width();++x)
                    image.setPixelColor(x,y,QColor((x*41)%256,(y*53)%256,170,(x+y)%3 ? 117 : 0));
            doc.activeLayer()->setImage(image);
            doc.activeLayer()->setOffset(origin);
            doc.activeLayer()->setRetainsOffCanvasPixels(true);
            const QPoint scaledOrigin(qRound(origin.x()*target.width()/8.0),qRound(origin.y()*target.height()/6.0));
            const QImage scaled = image.scaled(qMax(1,qRound(image.width()*target.width()/8.0)),
                qMax(1,qRound(image.height()*target.height()/6.0)),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            const QRect bounds = QRect(scaledOrigin,scaled.size()).united(QRect(QPoint(),target));
            QImage outside(bounds.size(),QImage::Format_ARGB32_Premultiplied);
            outside.fill(Qt::transparent);
            { QPainter painter(&outside); painter.drawImage(scaledOrigin-bounds.topLeft(),scaled); }
            const QImage finite = image.copy(QRect(-origin,doc.size())).scaled(target,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            doc.resize(target.width(),target.height());
            CHECK(doc.activeLayer()->offset() == bounds.topLeft() && doc.activeLayer()->image().size() == bounds.size(),
                  "resize keeps the full scaled bitmap/canvas union");
            CHECK(doc.flatten() == finite, "Source preserves exact finite-canvas alpha instead of double-compositing");
            bool untouched = true;
            for (int y=0;y<outside.height();++y)
                for (int x=0;x<outside.width();++x)
                    if (!QRect(QPoint(),target).contains(QPoint(x,y)+bounds.topLeft()) &&
                        doc.activeLayer()->image().pixel(x,y) != outside.pixel(x,y)) untouched = false;
            CHECK(untouched, "all outside-canvas pixels equal independently document-scaled bitmap");
        }
    }
}

static void ordinaryMoveAuthorityRegression() {
    SECTION("Only explicit retained state makes whole-layer Move nondestructive");
    for (QPoint origin : {QPoint(),QPoint(2,1),QPoint(-2,-1)}) {
        for (QSize buffer : {QSize(16,12),QSize(20,16),QSize(10,8)}) {
            Document doc(16,12);
            QImage before = makeImage(buffer.width(),buffer.height(),Qt::transparent);
            before.setPixelColor(4,3,QColor(200,60,120,137));
            doc.activeLayer()->setImage(before);
            doc.activeLayer()->setOffset(origin);
            CHECK(!doc.activeLayer()->retainsOffCanvasPixels(), "ordinary fixture has no retention provenance");
            QImage expected(buffer,QImage::Format_ARGB32_Premultiplied);
            expected.fill(Qt::transparent);
            { QPainter painter(&expected); painter.drawImage(3,2,before); }
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            MoveTool move;
            strokeTool(&move,canvas,QPointF(5,5),QPointF(8,7));
            CHECK(doc.activeLayer()->image() == expected && doc.activeLayer()->offset() == origin,
                  "ordinary layer bakes only movement delta even with a positioned/mismatched bitmap");
            const QImage visible = doc.flatten();
            doc.history().undo();
            CHECK(doc.activeLayer()->image() == before && doc.activeLayer()->offset() == origin &&
                  !doc.activeLayer()->retainsOffCanvasPixels(), "ordinary Move undo restores pixels and original origin");
            doc.history().redo();
            CHECK(doc.activeLayer()->image() == expected && doc.flatten() == visible &&
                  doc.activeLayer()->offset() == origin, "ordinary Move redo preserves alignment");
        }
    }
}

static void nativeRetentionCompatibility() {
    SECTION("Native retained state has a versioned format and reads PSW v1");
    QTemporaryDir directory;
    auto writeV1 = [&](const QString &path, const QImage &image, QPoint offset, int version) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_6_0);
        out << quint32(0x50535731) << qint32(version) << qint32(800) << qint32(600)
            << qint32(0) << qint32(1);
        out << QString("v1 fixture") << float(0.75) << bool(true) << bool(false)
            << qint32(0) << offset << image;
        return out.status() == QDataStream::Ok;
    };
    for (bool retained : {false, true}) {
        const QImage image = retained ? retainedPattern() : makeImage(800,600,Qt::red);
        const QPoint offset = retained ? QPoint(-50,-40) : QPoint();
        const QString path = directory.path() + (retained ? "/retained-v1.psw" : "/ordinary-v1.psw");
        CHECK(writeV1(path, image, offset, 1), "write an independent old-format fixture");
        Document loaded;
        CHECK(loaded.loadNative(path), "v1 files remain readable");
        CHECK(loaded.activeLayer()->retainsOffCanvasPixels() == retained,
              "v1 geometry inference distinguishes ordinary and oversized/positioned fixtures");
        CHECK(loaded.activeLayer()->image() == image && loaded.activeLayer()->offset() == offset &&
              loaded.activeLayer()->name() == "v1 fixture" && loaded.activeLayer()->opacity() == 0.75f,
              "v1 layer metadata and full bitmap preserved");
        // Expansion to equal dimensions must not erase the inferred retained
        // semantics of an old file with a positioned/oversized bitmap.
        if (retained) {
            loaded.pasteImage(image,"expand",true);
            loaded.setActiveLayer(0);
        }
        CanvasWidget canvas;
        canvas.setDocument(&loaded);
        MoveTool move;
        const QImage expanded = loaded.activeLayer()->image();
        strokeTool(&move,canvas,QPointF(300,200),QPointF(350,240));
        if (retained)
            CHECK(loaded.activeLayer()->image() == expanded && expanded.copy(image.rect()) == image &&
                  loaded.activeLayer()->offset().isNull(),
                  "v1 retained layer keeps full padded bitmap after canvas expansion and Move");
        else
            CHECK(loaded.activeLayer()->offset().isNull() && loaded.activeLayer()->image() != image &&
                  loaded.activeLayer()->image().pixelColor(0,0).alpha() == 0,
                  "v1 ordinary layer still uses the established baked move");
    }
    for (const auto &geometry : {std::make_pair(QSize(800,600),QPoint(3,-2)),
                                 std::make_pair(QSize(840,630),QPoint()),
                                 std::make_pair(QSize(840,630),QPoint(-3,2))}) {
        Document ordinary(800,600);
        const QImage before = makeImage(geometry.first.width(),geometry.first.height(),QColor(210,70,30,117));
        ordinary.activeLayer()->setImage(before);
        ordinary.activeLayer()->setOffset(geometry.second);
        CHECK(!ordinary.activeLayer()->retainsOffCanvasPixels(), "positioned ordinary fixture has explicit false retention");
        const QString path = directory.path()+"/positioned-ordinary.psw";
        CHECK(ordinary.saveNative(path), "save ordinary state that legacy geometry would infer as retained");
        Document restored;
        CHECK(restored.loadNative(path), "reload positioned ordinary document");
        CHECK(!restored.activeLayer()->retainsOffCanvasPixels(), "native roundtrip preserves explicit ordinary movement state");
        CHECK(restored.activeLayer()->image() == before && restored.activeLayer()->offset() == geometry.second,
              "native roundtrip preserves positioned ordinary bitmap and origin");
        QImage expected(before.size(),QImage::Format_ARGB32_Premultiplied);
        expected.fill(Qt::transparent);
        { QPainter painter(&expected); painter.drawImage(4,3,before); }
        CanvasWidget canvas;
        canvas.setDocument(&restored);
        MoveTool move;
        strokeTool(&move,canvas,QPointF(20,20),QPointF(24,23));
        CHECK(restored.activeLayer()->image() == expected && restored.activeLayer()->offset() == geometry.second,
              "ordinary Move remains pixel-baked after native roundtrip");
        restored.history().undo();
        CHECK(restored.activeLayer()->image() == before && !restored.activeLayer()->retainsOffCanvasPixels(),
              "native ordinary Move undo preserves explicit state");
    }
    const QString unknown = directory.path()+"/unknown.psw";
    CHECK(writeV1(unknown, retainedPattern(), QPoint(), 42), "write unknown-version fixture");
    Document untouched(23,17);
    const QImage before = untouched.flatten();
    CHECK(!untouched.loadNative(unknown), "reject unsupported native versions");
    CHECK(untouched.size() == QSize(23,17) && untouched.flatten() == before,
          "unknown format does not replace the live document");
    Document current(800,600);
    current.pasteImage(retainedPattern(),"Keep",false);
    const QString path = directory.path()+"/current.psw";
    CHECK(current.saveNative(path), "write current native format");
    QFile file(path);
    CHECK(file.open(QIODevice::ReadOnly), "open current native file");
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);
    quint32 magic = 0;
    qint32 version = 0;
    in >> magic >> version;
    CHECK(magic == quint32(0x50535731) && version == 2, "retained-state layout is explicitly version 2");
    file.close();
    Document ordinary(23,17);
    const QString ordinaryPath = directory.path()+"/ordinary-current.psw";
    CHECK(ordinary.saveNative(ordinaryPath), "save an ordinary document without retained state");
    QFile ordinaryFile(ordinaryPath);
    CHECK(ordinaryFile.open(QIODevice::ReadOnly), "open ordinary native output");
    QDataStream ordinaryStream(&ordinaryFile);
    ordinaryStream.setVersion(QDataStream::Qt_6_0);
    ordinaryStream >> magic >> version;
    CHECK(magic == quint32(0x50535731) && version == 1,
          "ordinary documents retain the legacy v1 output layout");
    for (bool retained : {false,true}) {
        // Both fixtures have identical dimensions and zero origin: only v2's
        // explicit flag can distinguish their movement provenance.
        const QString matchedPath = directory.path() + (retained ? "/matched-retained.psw" : "/matched-ordinary.psw");
        QFile matchedFile(matchedPath);
        CHECK(matchedFile.open(QIODevice::WriteOnly), "open independent matched-size v2 fixture");
        QDataStream out(&matchedFile);
        out.setVersion(QDataStream::Qt_6_0);
        const QImage image = makeImage(800,600,QColor(210,70,30,117));
        out << quint32(0x50535731) << qint32(2) << qint32(800) << qint32(600) << qint32(0) << qint32(1);
        out << QString("matched v2") << float(1) << true << false << qint32(0) << QPoint() << retained << image;
        CHECK(out.status() == QDataStream::Ok, "write independent v2 flag and bitmap");
        matchedFile.close();
        Document loaded;
        CHECK(loaded.loadNative(matchedPath), "read matched-size v2 fixture");
        CHECK(loaded.activeLayer()->retainsOffCanvasPixels() == retained && loaded.activeLayer()->image() == image &&
              loaded.activeLayer()->offset().isNull(), "v2 honours explicit provenance, never legacy geometry inference");
        CHECK(loaded.saveNative(matchedPath), "save matched-size native state again");
        Document reread;
        CHECK(reread.loadNative(matchedPath) && reread.activeLayer()->retainsOffCanvasPixels() == retained,
              "matched-size native save/load preserves retained or ordinary state");
    }
}

static void retainedMergeRegression() {
    SECTION("Merge Down composes retained bitmaps in a common document-space domain");
    for (bool keepArtwork : {false,true}) {
        for (const auto &offsets : {std::make_pair(QPoint(50,40),QPoint(-20,-10)),
                                   std::make_pair(QPoint(-50,-40),QPoint(20,10)),
                                   std::make_pair(QPoint(50,40),QPoint(950,-10))}) {
            Document doc(800,600);
            doc.activeLayer()->clear(Qt::transparent);
            const QImage lower = retainedPattern();
            QImage top = makeImage(870,650,Qt::transparent);
            { QPainter p(&top); p.fillRect(300,300,20,20,Qt::magenta);
              p.fillRect(840,610,20,20,Qt::red); }
            const QImage artwork = top;
            if (keepArtwork) {
                top.fill(Qt::white);
                QPainter p(&top); p.drawImage(0,0,artwork);
            }
            doc.pasteImage(lower,"lower",false);
            doc.activeLayer()->setOffset(offsets.first);
            doc.pasteImage(top,"upper",false);
            doc.activeLayer()->setOffset(offsets.second);
            doc.activeLayer()->setOpacity(0.75f);
            doc.selection().selectRect(QRect(10,20,30,40));
            const QImage mask = doc.selection().mask();
            const QImage before = doc.flatten();
            const QRect bounds = QRect(offsets.first,lower.size()).united(QRect(offsets.second,top.size()));
            QImage expected = makeImage(bounds.width(),bounds.height(),Qt::transparent);
            { QPainter p(&expected);
              p.drawImage(offsets.first-bounds.topLeft(),lower);
              p.setOpacity(0.75f);
              p.drawImage(offsets.second-bounds.topLeft(),artwork); }
            QImage expectedVisible = makeImage(800,600,Qt::transparent);
            { QPainter p(&expectedVisible); p.drawImage(bounds.topLeft(),expected); }
            doc.mergeLayerDown(2,keepArtwork);
            CHECK(doc.flatten() == expectedVisible, "both merge branches preserve artwork document positions");
            if (!keepArtwork)
                CHECK(doc.flatten() == before, "ordinary merge preserves exact visible composition");
            CHECK(doc.activeLayer()->image() == expected && doc.activeLayer()->offset() == bounds.topLeft(),
                  "merged buffer retains both complete domains, including artwork outside canvas");
            CHECK(doc.activeLayer()->retainsOffCanvasPixels(), "merged Keep layers retain nondestructive movement");
            CHECK(doc.layerCount() == 2 && doc.activeLayerIndex() == 1, "merge updates stack and active layer");
            doc.history().undo();
            CHECK(doc.layerCount() == 3 && doc.activeLayerIndex() == 2 && doc.flatten() == before,
                  "merge undo restores visible layer stack");
            CHECK(doc.layerAt(1)->image() == lower && doc.layerAt(1)->offset() == offsets.first &&
                  doc.layerAt(2)->image() == top && doc.layerAt(2)->offset() == offsets.second,
                  "merge undo restores full retained buffers and distinct origins");
            CHECK(doc.layerAt(1)->retainsOffCanvasPixels() && doc.layerAt(2)->retainsOffCanvasPixels() &&
                  doc.selection().mask() == mask, "merge undo restores retained semantics and selection");
            doc.history().redo();
            CHECK(doc.activeLayer()->image() == expected && doc.activeLayer()->offset() == bounds.topLeft() &&
                  doc.flatten() == expectedVisible, "merge redo restores the common domain exactly");
        }
    }
}

// =====================================================================
// Paint.NET .pdn import, checked against files saved by Paint.NET itself
// (fixtures and reference renders from the MIT-licensed pypdn project).
static int maxRgbDifference(const QImage &a, const QImage &b) {
    if (a.size() != b.size()) return INT_MAX;
    const QImage x = a.convertToFormat(QImage::Format_ARGB32);
    const QImage y = b.convertToFormat(QImage::Format_ARGB32);
    int worst = 0;
    for (int row = 0; row < x.height(); ++row) {
        const QRgb *p = reinterpret_cast<const QRgb *>(x.constScanLine(row));
        const QRgb *q = reinterpret_cast<const QRgb *>(y.constScanLine(row));
        for (int col = 0; col < x.width(); ++col) {
            worst = std::max({worst, std::abs(qRed(p[col]) - qRed(q[col])),
                              std::abs(qGreen(p[col]) - qGreen(q[col])),
                              std::abs(qBlue(p[col]) - qBlue(q[col]))});
        }
    }
    return worst;
}

static void pdnImportRegression() {
    SECTION("Paint.NET .pdn import");
    const QString dir = QStringLiteral(PAINTSW_TEST_DATA_DIR "/pdn/");

    Pdn::ImageData pdn;
    QString error;
    CHECK(Pdn::isPdnFile(dir + "Untitled3.pdn"), "recognise the PDN3 signature");
    CHECK(!Pdn::isPdnFile(dir + "layerVisibleTest1.png"), "a PNG is not a .pdn");
    const bool read = Pdn::read(dir + "Untitled3.pdn", pdn, &error);
    CHECK(read, qPrintable("read a Paint.NET 4 document: " + error));
    if (read) {
        CHECK(pdn.width == 800 && pdn.height == 600, "document size comes from the file");
        CHECK(pdn.layers.size() == 2, "both layers are read");
    }
    if (read && pdn.layers.size() == 2) {
        const Pdn::LayerData &background = pdn.layers[0];
        const Pdn::LayerData &top = pdn.layers[1];
        CHECK(background.name == "Background" && top.name == "Layer 2", "layer names are kept");
        CHECK(background.visible && top.visible, "visibility is kept");
        CHECK(qAbs(top.opacity - 161 / 255.0f) < 1e-4f && background.opacity == 1.0f,
              "layer opacity is kept");
        CHECK(top.blendMode == BlendMode::Additive && background.blendMode == BlendMode::Normal,
              "blend modes are kept");
        const QImage expected = QImage(dir + "layerVisibleTest1.png").convertToFormat(QImage::Format_ARGB32);
        bool samePixels = background.image.size() == expected.size();
        for (int y = 0; samePixels && y < expected.height(); ++y)
            samePixels = std::memcmp(background.image.constScanLine(y), expected.constScanLine(y),
                                     size_t(expected.width()) * 4) == 0;
        CHECK(samePixels, "background pixels match Paint.NET exactly");
        CHECK(maxRgbDifference(top.image, QImage(dir + "layerVisibleTest2.png")) == 0,
              "top layer colours match Paint.NET");
    }

    // Through Document::load, as File > Open does.
    Document doc(1, 1);
    CHECK(doc.load(dir + "Untitled3.pdn"), "Document opens a .pdn");
    CHECK(doc.layerCount() == 2 && doc.width() == 800 && doc.height() == 600,
          "opened document keeps every layer");
    CHECK(doc.activeLayerIndex() == doc.layerCount() - 1, "the top layer is active, like Paint.NET");
    CHECK(doc.layerAt(1) && doc.layerAt(1)->blendMode() == BlendMode::Additive,
          "opened layer keeps its blend mode");
    CHECK(!doc.isModified() && doc.history().canUndo() == false, "a freshly opened .pdn is clean");
    CHECK(Document::isOpenOnlyFormat(doc.filePath()), "Save is redirected away from .pdn");

    // Paint.NET 3.5 files store the blend op as an object instead of an enum.
    Document old(1, 1);
    CHECK(old.load(dir + "oldPDN3510.pdn"), "open a Paint.NET 3.5 document");
    CHECK(old.layerCount() == 2, "Paint.NET 3.5 layers are read");
    for (int i = 0; i < old.layerCount(); ++i) old.layerAt(i)->setVisible(true);
    CHECK(maxRgbDifference(old.flattenVisible(), QImage(dir + "oldPDN3510.png")) <= 2,
          "Paint.NET 3.5 document renders like Paint.NET");

    // Damaged files must fail cleanly, never crash or hang.
    QFile file(dir + "Untitled3.pdn");
    CHECK(file.open(QIODevice::ReadOnly), "read fixture bytes");
    const QByteArray bytes = file.readAll();
    bool truncatedRejected = true;
    for (int cut = 0; cut < bytes.size(); cut += 97) {
        Pdn::ImageData ignored;
        truncatedRejected &= !Pdn::read(bytes.left(cut), ignored);
    }
    CHECK(truncatedRejected, "truncated files are rejected");
    quint32 seed = 12345;
    for (int round = 0; round < 300; ++round) {
        QByteArray damaged = bytes;
        for (int k = 0; k < 4; ++k) {
            seed = seed * 1664525u + 1013904223u;
            damaged[int(seed % quint32(damaged.size()))] = char(seed >> 24);
        }
        Pdn::ImageData ignored;
        Pdn::read(damaged, ignored);
    }
    CHECK(true, "corrupted files do not crash the reader");
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    resizeStraightAlphaCharacterization();
    retainedCropRegression();
    retainedExpansionRegression();
    pasteExpansionBoundsRegression();
    retainedOrthogonalRegression();
    retainedResizeRegression();
    resizeStampCharacterization();
    ordinaryMoveAuthorityRegression();
    nativeRetentionCompatibility();
    retainedMergeRegression();
    pdnImportRegression();

    // ---------- TOOLS ----------
    SECTION("Tools draw into the active layer");
    {
        auto testDraw = [](const char *name, Tool *tool, bool useTwoPoints = true,
                           bool commitAfter = false) {
            Document doc(64, 64);
            doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::red);
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            QImage before = doc.activeLayer()->image().copy();
            if (useTwoPoints) strokeTool(tool, canvas, QPointF(10, 10), QPointF(50, 50));
            else {
                QMouseEvent e = pressEv(QPointF(32, 32));
                tool->mousePressEvent(QPointF(32, 32), &e, canvas);
                QMouseEvent r = releaseEv(QPointF(32, 32));
                tool->mouseReleaseEvent(QPointF(32, 32), &r, canvas);
            }
            // Edit-before-commit tools (Shape, Line) only rasterise on commit.
            if (commitAfter) tool->deactivate(canvas);
            bool changed = imagesDiffer(before, doc.activeLayer()->image());
            CHECK(changed, name);
        };
        BrushTool brush;        testDraw("Brush modifies pixels", &brush);
        PencilTool pencil;      testDraw("Pencil modifies pixels", &pencil);
        FillTool fill;          testDraw("Fill modifies pixels", &fill, false);
        ShapeTool shape;        testDraw("Shape modifies pixels", &shape, true, /*commitAfter*/true);
        GradientTool grad;      testDraw("Gradient modifies pixels", &grad);
        // Eraser needs opaque content to erase.
        {
            Document doc(64, 64); doc.activeLayer()->clear(Qt::black);
            CanvasWidget canvas; canvas.setDocument(&doc);
            QImage before = doc.activeLayer()->image().copy();
            EraserTool eraser; strokeTool(&eraser, canvas, QPointF(10,10), QPointF(50,50));
            CHECK(imagesDiffer(before, doc.activeLayer()->image()), "Eraser modifies pixels");
            // eraser should reduce alpha somewhere
            bool madeTransparent = false;
            QImage &img = doc.activeLayer()->image();
            for (int y=0;y<64 && !madeTransparent;++y) for (int x=0;x<64;++x)
                if (img.pixelColor(x,y).alpha() < 255) { madeTransparent = true; break; }
            CHECK(madeTransparent, "Eraser produces transparency");
        }
        // Recolor: needs a target colour present.
        {
            Document doc(64, 64); doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::blue);
            CanvasWidget canvas; canvas.setDocument(&doc);
            QImage before = doc.activeLayer()->image().copy();
            RecolorTool rc; rc.setTolerance(255);
            strokeTool(&rc, canvas, QPointF(10,10), QPointF(50,50));
            CHECK(imagesDiffer(before, doc.activeLayer()->image()), "Recolor modifies pixels");
        }
        // Clone stamp: set source (alt/right) then paint.
        {
            Document doc(64, 64); doc.activeLayer()->clear(Qt::white);
            QPainter p(&doc.activeLayer()->image()); p.fillRect(0,0,20,20,Qt::green); p.end();
            CanvasWidget canvas; canvas.setDocument(&doc);
            CloneStampTool clone;
            QMouseEvent src = pressEv(QPointF(10,10), Qt::RightButton);
            clone.mousePressEvent(QPointF(10,10), &src, canvas);   // set source
            QImage before = doc.activeLayer()->image().copy();
            strokeTool(&clone, canvas, QPointF(40,40), QPointF(45,45));
            CHECK(imagesDiffer(before, doc.activeLayer()->image()), "Clone stamp modifies pixels");
        }
        // During a clone stroke, the sampled-source crosshair follows the
        // source offset while the original source crosshair stays fixed.
        {
            Document doc(100, 100); doc.activeLayer()->clear(Qt::white);
            CanvasWidget canvas; canvas.setDocument(&doc); canvas.setZoom(1.0);
            canvas.setPan(QPointF(0, 0));
            CloneStampTool clone;
            clone.setBrushSize(1);
            QMouseEvent src = pressEv(QPointF(30, 30), Qt::RightButton);
            clone.mousePressEvent(QPointF(30, 30), &src, canvas);
            QMouseEvent press = pressEv(QPointF(40, 40));
            clone.mousePressEvent(QPointF(40, 40), &press, canvas);
            QMouseEvent move = moveEv(QPointF(40, 35));
            clone.mouseMoveEvent(QPointF(40, 35), &move, canvas);

            QImage overlay(100, 100, QImage::Format_ARGB32);
            overlay.fill(Qt::white);
            QPainter painter(&overlay);
            clone.drawOverlay(painter, canvas);
            painter.end();

            CHECK(overlay.pixelColor(58, 45) == QColor(Qt::blue),
                  "Clone overlay shows blue crosshair at the sampled source");
            CHECK(overlay.pixelColor(58, 50) == QColor(Qt::red),
                  "Clone overlay keeps red crosshair at the original source");
        }
    }

    // ---------- RECOLOR & PENCIL OPTIONS ----------
    SECTION("Recolor and Pencil expose Paint.NET options");
    {
        // Recolor, Sampling = Secondary colour: only pixels matching the
        // secondary colour are recolored, not arbitrary clicked colours.
        {
            Document doc(64, 64); doc.activeLayer()->clear(Qt::white);
            // Paint the left half green; leave the right half white.
            { QPainter p(&doc.activeLayer()->image()); p.fillRect(0, 0, 32, 64, Qt::green); }
            doc.setPrimaryColor(Qt::red);
            doc.setSecondaryColor(Qt::green);
            CanvasWidget canvas; canvas.setDocument(&doc);
            RecolorTool rc;
            rc.setSampleSecondary(true);   // target = secondary (green)
            rc.setTolerance(0);            // exact match only
            rc.setBrushSize(300);          // one dab covers the whole canvas
            strokeTool(&rc, canvas, QPointF(10, 10), QPointF(50, 50));
            QColor greenSide = doc.activeLayer()->image().pixelColor(10, 32);
            QColor whiteSide = doc.activeLayer()->image().pixelColor(50, 32);
            CHECK(greenSide.red() > 200 && greenSide.green() < 60,
                  "Recolor Sampling=Secondary: green pixels become the primary (red)");
            CHECK(whiteSide == QColor(Qt::white),
                  "Recolor Sampling=Secondary: non-secondary (white) pixels are untouched");
        }

        // Recolor Hardness: a soft tip changes edge pixels less than the centre.
        {
            Document doc(64, 64); doc.activeLayer()->clear(Qt::green);
            doc.setPrimaryColor(Qt::red);
            CanvasWidget canvas; canvas.setDocument(&doc);
            RecolorTool rc;
            rc.setSampleSecondary(false);   // target = clicked pixel (green)
            rc.setHardness(0);              // maximum soft edge
            rc.setBrushSize(40);            // radius 20
            QMouseEvent e = pressEv(QPointF(32, 32));
            rc.mousePressEvent(QPointF(32, 32), &e, canvas);
            QMouseEvent r = releaseEv(QPointF(32, 32));
            rc.mouseReleaseEvent(QPointF(32, 32), &r, canvas);
            int centreRed = doc.activeLayer()->image().pixelColor(32, 32).red();
            int edgeRed   = doc.activeLayer()->image().pixelColor(32, 49).red();  // near the rim
            CHECK(centreRed > edgeRed + 40,
                  "Recolor Hardness: centre is recolored more strongly than the edge");
        }

        // Pencil blend mode: Multiply differs from Normal on a mid-grey ground.
        {
            const QColor grey(128, 128, 128);
            auto pencilStroke = [&](int blendMode) {
                Document doc(64, 64); doc.activeLayer()->clear(grey);
                doc.setPrimaryColor(grey);
                CanvasWidget canvas; canvas.setDocument(&doc);
                PencilTool pencil;
                pencil.setBlendMode(blendMode);
                strokeTool(&pencil, canvas, QPointF(10, 10), QPointF(40, 40));
                return doc.activeLayer()->image().pixelColor(10, 10);
            };
            QColor normal   = pencilStroke(0);    // Normal
            QColor multiply = pencilStroke(1);    // Multiply
            CHECK(normal == grey, "Pencil Normal writes the source colour directly");
            CHECK(multiply.red() < normal.red(),
                  "Pencil Multiply darkens the pixel, differing from Normal");
        }
    }

    // ---------- GRADIENT TOOL OPTIONS ----------
    SECTION("Gradient tool exposes Paint.NET options");
    {
        GradientTool grad;
        // New gradient types exist and round-trip through the setter.
        grad.setGradientType(GradientType::LinearReflected);
        CHECK(grad.gradientType() == GradientType::LinearReflected, "LinearReflected type");
        grad.setGradientType(GradientType::Spiral);
        CHECK(grad.gradientType() == GradientType::Spiral, "Spiral type");
        // Repeat mode: default None, settable.
        CHECK(grad.repeatMode() == GradientRepeat::None, "Repeat defaults to None");
        grad.setRepeatMode(GradientRepeat::Reflect);
        CHECK(grad.repeatMode() == GradientRepeat::Reflect, "Repeat mode settable");
        // Transparency mode: default off, settable.
        CHECK(!grad.transparencyMode(), "Transparency defaults off");
        grad.setTransparencyMode(true);
        CHECK(grad.transparencyMode(), "Transparency mode settable");
        // Transparency mode paints varying alpha into the layer.
        {
            Document doc(64, 64); doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::red);
            CanvasWidget canvas; canvas.setDocument(&doc);
            GradientTool g; g.setTransparencyMode(true);
            g.setGradientType(GradientType::Linear);
            strokeTool(&g, canvas, QPointF(2,2), QPointF(60,60));
            bool madeTransparent = false;
            QImage &img = doc.activeLayer()->image();
            for (int y=0;y<64 && !madeTransparent;++y) for (int x=0;x<64;++x)
                if (img.pixelColor(x,y).alpha() < 255) { madeTransparent = true; break; }
            CHECK(madeTransparent, "Transparency gradient varies alpha");
        }
        // Right-button drag draws (uses secondary as base) without crashing.
        {
            Document doc(64, 64); doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::red); doc.setSecondaryColor(Qt::blue);
            CanvasWidget canvas; canvas.setDocument(&doc);
            GradientTool g;
            QImage before = doc.activeLayer()->image().copy();
            QMouseEvent p = pressEv(QPointF(4,4), Qt::RightButton);
            g.mousePressEvent(QPointF(4,4), &p, canvas);
            QMouseEvent r = releaseEv(QPointF(58,58));
            g.mouseReleaseEvent(QPointF(58,58), &r, canvas);
            CHECK(imagesDiffer(before, doc.activeLayer()->image()), "Right-button gradient draws");
        }
    }

    // ---------- SELECTION-AWARE DRAWING ----------
    SECTION("Selection confines drawing");
    {
        Document doc(64, 64);
        doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::red);
        doc.selection().selectRect(QRect(0, 0, 20, 20), SelectionMode::Replace);
        CanvasWidget canvas; canvas.setDocument(&doc);
        BrushTool brush; brush.setBrushSize(6);
        strokeTool(&brush, canvas, QPointF(5, 5), QPointF(55, 55));
        QImage &img = doc.activeLayer()->image();
        // Inside selection near (5,5) should be reddish; outside (say 50,50) untouched white.
        bool insidePainted = img.pixelColor(6, 6).red() > 150 && img.pixelColor(6,6).blue() < 120;
        bool outsideClean = img.pixelColor(50, 50) == QColor(Qt::white);
        CHECK(insidePainted, "Brush paints inside the selection");
        CHECK(outsideClean, "Brush does NOT paint outside the selection");
    }

    // ---------- SELECTION OPERATIONS ----------
    SECTION("Selection operations");
    {
        Selection sel(64, 64);
        CHECK(!sel.hasSelection(), "New selection is empty");
        sel.selectAll();
        CHECK(sel.hasSelection() && sel.isSelected(32,32), "selectAll selects everything");
        sel.clear();
        CHECK(!sel.hasSelection(), "clear deselects");
        sel.selectRect(QRect(10,10,20,20), SelectionMode::Replace);
        CHECK(sel.isSelected(15,15) && !sel.isSelected(50,50), "selectRect");
        sel.invert();
        CHECK(!sel.isSelected(15,15) && sel.isSelected(50,50), "invert");
        // add / subtract / intersect
        Selection s2(64,64);
        s2.selectRect(QRect(0,0,32,64), SelectionMode::Replace);
        s2.selectRect(QRect(32,0,32,64), SelectionMode::Add);
        CHECK(s2.isSelected(10,10) && s2.isSelected(50,10), "Add mode unions");
        s2.selectRect(QRect(0,0,32,64), SelectionMode::Subtract);
        CHECK(!s2.isSelected(10,10) && s2.isSelected(50,10), "Subtract mode removes");
        Selection s3(64,64);
        s3.selectRect(QRect(0,0,40,64), SelectionMode::Replace);
        s3.selectRect(QRect(30,0,34,64), SelectionMode::Intersect);
        CHECK(s3.isSelected(35,10) && !s3.isSelected(10,10), "Intersect mode");
        // Invert (XOR) mode: overlap cancels, the rest unions.
        Selection s5(64,64);
        s5.selectRect(QRect(0,0,40,64), SelectionMode::Replace);
        s5.selectRect(QRect(30,0,34,64), SelectionMode::Invert);
        CHECK(s5.isSelected(10,10) && s5.isSelected(50,10) && !s5.isSelected(35,10),
              "Invert mode XORs (overlap removed, rest kept)");

        // Modifier mapping must match Paint.NET exactly: Ctrl=add, Alt=subtract,
        // Alt+right=intersect, Ctrl+right=invert, nothing=replace.
        CHECK(selectionModeFor(Qt::NoModifier, Qt::LeftButton) == SelectionMode::Replace, "no modifier = replace");
        CHECK(selectionModeFor(Qt::ControlModifier, Qt::LeftButton) == SelectionMode::Add, "Ctrl = add");
        CHECK(selectionModeFor(Qt::AltModifier, Qt::LeftButton) == SelectionMode::Subtract, "Alt = subtract");
        CHECK(selectionModeFor(Qt::AltModifier, Qt::RightButton) == SelectionMode::Intersect, "Alt+right = intersect");
        CHECK(selectionModeFor(Qt::ControlModifier, Qt::RightButton) == SelectionMode::Invert, "Ctrl+right = invert");
        // The old Shift mapping must be gone.
        CHECK(selectionModeFor(Qt::ShiftModifier, Qt::LeftButton) == SelectionMode::Replace, "Shift no longer adds");

        // Tolerance is now a 0..100 percentage (Paint.NET), mapped to the 0..255
        // colour-distance basis the flood/wand/recolor code compares against.
        BrushTool tolTool;   // any Tool exposes the shared tolerance
        tolTool.setTolerance(0);   CHECK(tolTool.toleranceDistance() == 0, "0% tolerance = 0 distance");
        tolTool.setTolerance(100); CHECK(tolTool.toleranceDistance() == 255, "100% tolerance = 255 distance");
        tolTool.setTolerance(50);  CHECK(tolTool.toleranceDistance() >= 126 && tolTool.toleranceDistance() <= 128,
                                         "50% tolerance = ~127 distance");
        tolTool.setTolerance(500); CHECK(tolTool.tolerance() == 100, "tolerance clamps to 100%");

        // feather / expand / contract run without crashing and keep selection
        Selection s4(64,64); s4.selectRect(QRect(20,20,24,24), SelectionMode::Replace);
        s4.expand(3);  CHECK(s4.isSelected(18,32), "expand grows selection");
        s4.contract(3); CHECK(s4.hasSelection(), "contract keeps selection");
        s4.feather(2);  CHECK(s4.hasSelection(), "feather keeps selection");
    }

    // ---------- LAYERS ----------
    SECTION("Layer operations");
    {
        Document doc(32, 32);
        CHECK(doc.layerCount() == 1, "starts with 1 layer");
        doc.addLayer();
        CHECK(doc.layerCount() == 2, "addLayer");
        doc.duplicateLayer(doc.activeLayerIndex());
        CHECK(doc.layerCount() == 3, "duplicateLayer");
        doc.removeLayer(0);
        CHECK(doc.layerCount() == 2, "removeLayer");
        doc.mergeLayerDown(1);
        CHECK(doc.layerCount() == 1, "mergeLayerDown");
        doc.addLayer(); doc.addLayer();
        int before = doc.layerCount();
        doc.flattenImage();
        CHECK(doc.layerCount() == 1 && before == 3, "flattenImage collapses to 1");
        // reorder
        Document d2(16,16); d2.addLayer(); d2.addLayer();
        d2.layerAt(0)->setName("A"); d2.layerAt(1)->setName("B"); d2.layerAt(2)->setName("C");
        d2.reorderLayers({2,1,0}, 0);
        CHECK(d2.layerAt(0)->name()=="C" && d2.layerAt(2)->name()=="A", "reorderLayers");
    }

    // ---------- REGIONAL RENDERING ----------
    SECTION("Regional flatten matches full flatten");
    {
        Document doc(48, 36);
        doc.activeLayer()->clear(QColor(40, 70, 110, 255));
        QImage topImage(22, 18, QImage::Format_ARGB32_Premultiplied);
        topImage.fill(QColor(200, 80, 30, 210));
        const int topIndex = doc.addLayer(topImage);
        doc.layerAt(topIndex)->setOffset(QPoint(17, -4));
        doc.layerAt(topIndex)->setOpacity(0.65f);
        doc.layerAt(topIndex)->setBlendMode(BlendMode::Multiply);

        const QImage full = doc.flattenVisible();
        for (const QRect &tile : {QRect(0, 0, 16, 14), QRect(8, 6, 24, 20),
                                  QRect(30, 16, 18, 20)}) {
            CHECK(doc.flattenVisible(tile) == full.copy(tile),
                  "regional flatten matches full composite");
        }
    }

    // ---------- SMART MERGE (keep both artworks) ----------
    SECTION("Smart merge keeps artwork from both layers");
    {
        Document doc(40, 40);
        // Bottom layer: white with a BLUE square in the top-left corner area.
        doc.activeLayer()->clear(Qt::white);
        { QPainter p(&doc.activeLayer()->image()); p.fillRect(2,2,12,12, QColor(0,0,255)); p.end(); }
        // Top layer: white with a RED square in the bottom-right, elsewhere white.
        int top = doc.addLayer();   // white by default
        { QPainter p(&doc.layerAt(top)->image()); p.fillRect(26,26,12,12, QColor(255,0,0)); p.end(); }

        // Normal composite would hide the blue (opaque white top). keepArtwork
        // knocks out the white so BOTH squares survive.
        doc.mergeLayerDown(top, /*keepArtwork=*/true);
        CHECK(doc.layerCount()==1, "smart merge collapses to 1 layer");
        QColor blue = doc.layerAt(0)->image().pixelColor(6, 6);
        QColor red  = doc.layerAt(0)->image().pixelColor(31, 31);
        CHECK(blue.blue()>200 && blue.red()<60, "bottom artwork (blue) is kept");
        CHECK(red.red()>200 && red.blue()<60, "top artwork (red) is kept");
    }

    // ---------- BLEND MODES ----------
    SECTION("Blend modes composite");
    {
        for (BlendMode m : Layer::allBlendModes()) {
            Layer top(16,16,"t");
            QImage ti = makeImage(16,16, QColor(180,120,60,255));
            top.setImage(ti); top.setBlendMode(m);
            QImage below = makeImage(16,16, QColor(90,90,90,255));
            QImage r = top.composited(below);
            CHECK(r.size()==below.size() && !r.isNull(),
                  qPrintable(QString("Blend mode %1 produces output").arg(Layer::blendModeName(m))));
        }
    }

    // ---------- BLEND MODE FORMULAS (Paint.NET fidelity) ----------
    SECTION("Blend mode formulas match Paint.NET");
    {
        // Composite a known top colour over a known base and check the result of
        // the modes the task called out: Xor, Overlay, Color Burn, Color Dodge.
        const QColor base(90, 90, 90, 255);
        const QColor top(180, 120, 60, 255);
        auto blended = [&](BlendMode m) {
            Layer t(16, 16, "t");
            t.setImage(makeImage(16, 16, top));
            t.setBlendMode(m);
            QImage r = t.composited(makeImage(16, 16, base));
            return r.pixelColor(8, 8);
        };
        auto near = [](int a, int b, int tol) { return std::abs(a - b) <= tol; };

        // Xor: per-channel bitwise XOR of the colour bytes (exact).
        QColor x = blended(BlendMode::Xor);
        CHECK(near(x.red(),   90 ^ 180, 1) && near(x.green(), 90 ^ 120, 1) &&
              near(x.blue(),  90 ^ 60,  1),
              "Xor is a per-channel bitwise XOR of the two colours");

        // Overlay: base<128 -> 2*B*T/255 per channel.
        QColor o = blended(BlendMode::Overlay);
        CHECK(near(o.red(), 2*90*180/255, 4) && near(o.green(), 2*90*120/255, 4) &&
              near(o.blue(), 2*90*60/255, 4),
              "Overlay uses the standard overlay formula");

        // Color Burn: 255 - (255-B)*255/T, clamped to 0.
        QColor cb = blended(BlendMode::ColorBurn);
        CHECK(near(cb.red(), 21, 4) && cb.green() <= 4 && cb.blue() <= 4,
              "Color Burn uses the standard color-burn formula");

        // Color Dodge: min(255, B*255/(255-T)).
        QColor cd = blended(BlendMode::ColorDodge);
        CHECK(cd.red() >= 251 && near(cd.green(), 170, 4) && near(cd.blue(), 118, 4),
              "Color Dodge uses the standard color-dodge formula");
    }

    // ---------- MOVE LAYER TO TOP / BOTTOM ----------
    SECTION("Move layer to very top / bottom");
    {
        // Ctrl+click on the panel's move buttons calls moveLayer(idx, top/bottom);
        // verify that reordering directly.
        Document doc(16, 16);                 // layer 0 = "Background"
        doc.layerAt(0)->setName("A");
        int b = doc.addLayer(); doc.layerAt(b)->setName("B");
        int c = doc.addLayer(); doc.layerAt(c)->setName("C");
        // Stack (bottom->top): A, B, C
        doc.moveLayer(0, doc.layerCount() - 1);       // send A to the very top
        CHECK(doc.layerAt(0)->name()=="B" && doc.layerAt(1)->name()=="C" &&
              doc.layerAt(2)->name()=="A", "move-to-top reorders correctly");
        doc.moveLayer(2, 0);                           // send A back to the bottom
        CHECK(doc.layerAt(0)->name()=="A" && doc.layerAt(1)->name()=="B" &&
              doc.layerAt(2)->name()=="C", "move-to-bottom reorders correctly");
    }

    // ---------- UNDO / REDO ----------
    SECTION("Undo / redo");
    {
        Document doc(48,48); doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::red);
        CanvasWidget canvas; canvas.setDocument(&doc);
        QImage clean = doc.activeLayer()->image().copy();
        BrushTool brush; strokeTool(&brush, canvas, QPointF(5,5), QPointF(40,40));
        QImage painted = doc.activeLayer()->image().copy();
        CHECK(imagesDiffer(clean, painted), "stroke changed the layer");
        CHECK(doc.history().canUndo(), "history has an undo entry");
        doc.history().undo();
        CHECK(!imagesDiffer(clean, doc.activeLayer()->image()), "undo restores clean image");
        CHECK(doc.history().canRedo(), "history has a redo entry");
        doc.history().redo();
        CHECK(!imagesDiffer(painted, doc.activeLayer()->image()), "redo re-applies stroke");
        // structural undo: duplicate then undo
        int n = doc.layerCount(); doc.duplicateLayer(0);
        CHECK(doc.layerCount()==n+1, "duplicate added a layer");
        doc.history().undo();
        CHECK(doc.layerCount()==n, "undo removes the duplicated layer");
    }

    // ---------- SAVE / LOAD ----------
    SECTION("Save / load round-trip");
    {
        Document doc(24,24);
        doc.activeLayer()->clear(Qt::transparent);
        QPainter p(&doc.activeLayer()->image());
        p.fillRect(6,6,12,12, QColor(255,0,0,255)); p.end();
        QString png = QDir::tempPath() + "/_pdtest.png", jpg = QDir::tempPath() + "/_pdtest.jpg";
        CHECK(doc.save(png), "save PNG");
        CHECK(doc.save(jpg), "save JPG");
        Document loaded(png);
        CHECK(loaded.width()==24 && loaded.height()==24, "PNG reloads at correct size");
        QImage flat = loaded.flatten();
        CHECK(flat.pixelColor(0,0).alpha()==0, "PNG preserved transparency");
        CHECK(flat.pixelColor(12,12).red()>200, "PNG preserved the red square");
        Document jl(jpg);
        CHECK(jl.flatten().pixelColor(0,0).red()>230, "JPG filled transparent with white");
    }

    // ---------- EFFECTS ----------
    SECTION("Effects run and return valid output");
    {
        QImage src = makeImage(48, 48, QColor(120,160,200,255));
        // add some structure so effects have something to work on
        { QPainter p(&src); p.fillRect(10,10,28,28, QColor(240,60,60)); p.end(); }

        auto testEffect = [&](const char *name, Effect *fx) {
            QImage out = fx->apply(src);
            bool ok = !out.isNull() && out.width()>0 && out.height()>0;
            CHECK(ok, name);
            delete fx;
        };
        testEffect("Blur", new BlurEffect);
        testEffect("Sharpen", new SharpenEffect);
        testEffect("Noise", new NoiseEffect);
        testEffect("Emboss", new EmbossEffect);
        testEffect("EdgeDetect", new EdgeDetectEffect);
        testEffect("OilPaint", new OilPaintEffect);
        testEffect("Pixelate", new PixelateEffect);
        testEffect("MotionBlur", new MotionBlurEffect);
        testEffect("RadialBlur", new RadialBlurEffect);
        testEffect("ZoomBlur", new ZoomBlurEffect);
        testEffect("SurfaceBlur", new SurfaceBlurEffect);
        testEffect("Unfocus", new UnfocusEffect);
        testEffect("Fragment", new FragmentEffect);
        testEffect("Bulge", new BulgeEffect);
        testEffect("Twist", new TwistEffect);
        testEffect("FrostedGlass", new FrostedGlassEffect);
        testEffect("Crystalize", new CrystalizeEffect);
        testEffect("Tile", new TileEffect);
        testEffect("Dents", new DentsEffect);
        testEffect("PolarInversion", new PolarInversionEffect);
        testEffect("Median", new MedianEffect);
        testEffect("ReduceNoise", new ReduceNoiseEffect);
        testEffect("Quantize", new QuantizeEffect);
        testEffect("Glow", new GlowEffect);
        testEffect("RedEyeRemove", new RedEyeRemoveEffect);
        testEffect("SoftenPortrait", new SoftenPortraitEffect);
        testEffect("Vignette", new VignetteEffect);
        testEffect("Turbulence", new TurbulenceEffect);
        testEffect("Relief", new ReliefEffect);
        testEffect("Outline", new OutlineEffect);
        testEffect("Morphology", new MorphologyEffect);
    }

    // Effects that must visibly change the image at default params
    SECTION("Key effects actually change the image");
    {
        QImage src = makeImage(48, 48, QColor(120,160,200,255));
        { QPainter p(&src); p.fillRect(10,10,28,28, QColor(240,60,60)); p.end(); }
        auto changes = [&](const char *name, Effect *fx) {
            QImage out = fx->apply(src);
            CHECK(imagesDiffer(src, out.convertToFormat(src.format())), name);
            delete fx;
        };
        changes("Blur changes image", new BlurEffect);
        changes("Invert-style EdgeDetect changes image", new EdgeDetectEffect);
        changes("Emboss changes image", new EmbossEffect);
        changes("Pixelate changes image", new PixelateEffect);
        // Relief must not be flat gray (regression from the truncation bug)
        {
            ReliefEffect r; QImage out = r.apply(src);
            bool allGray = true;
            for (int y=0;y<out.height()&&allGray;++y) for (int x=0;x<out.width();++x) {
                QColor c = out.pixelColor(x,y);
                if (!(c.red()==128&&c.green()==128&&c.blue()==128)) { allGray=false; break; }
            }
            CHECK(!allGray, "Relief is not flat gray at default angle");
        }
        // Turbulence must match input size
        {
            TurbulenceEffect t; QImage out = t.apply(src);
            CHECK(out.size()==src.size(), "Turbulence output matches input size");
        }
    }

    // ---------- ADJUSTMENTS ----------
    SECTION("Adjustments run and return valid output");
    {
        QImage src = makeImage(32, 32, QColor(120,160,200,255));
        { QPainter p(&src); p.fillRect(4,4,10,10, QColor(240,60,60)); p.end(); }
        auto testAdj = [&](const char *name, Adjustment *a, bool mustChange) {
            QImage out = a->apply(src);
            CHECK(!out.isNull(), name);
            if (mustChange)
                CHECK(imagesDiffer(src, out.convertToFormat(src.format())),
                      qPrintable(QString("%1 changes the image").arg(name)));
            delete a;
        };
        { BrightnessContrast a; a.setBrightness(40); a.setContrast(20);
          testAdj("BrightnessContrast", new BrightnessContrast(a), true); }
        { HueSaturation a; a.setHue(60);
          testAdj("HueSaturation", new HueSaturation(a), true); }
        { Levels a; a.setInputBlack(30); a.setInputWhite(220);
          testAdj("Levels", new Levels(a), true); }
        { Curves a; QVector<QPointF> pts{{0,40},{128,180},{255,255}}; a.setControlPoints(pts);
          testAdj("Curves", new Curves(a), true); }
        testAdj("InvertColors", new InvertColors, true);
        { Sepia a; a.setIntensity(90); testAdj("Sepia", new Sepia(a), true); }
        { Posterize a; a.setLevels(3); testAdj("Posterize", new Posterize(a), true); }
        { Threshold a; a.setThreshold(128); testAdj("Threshold", new Threshold(a), true); }
        { ColorBalance a; a.setCyanRed(40); testAdj("ColorBalance", new ColorBalance(a), true); }
    }

    SECTION("Curves canvas preview, selection, transparency and grouped history");
    {
        Document doc(4, 4);
        QImage source(4, 4, QImage::Format_ARGB32_Premultiplied);
        source.fill(QColor(64, 96, 128, 128));
        source.setPixel(0, 0, 0);
        doc.layerAt(0)->setImage(source);
        doc.addLayer(source, "Second layer");
        doc.layerAt(1)->setOffset(QPoint(1, 0));
        doc.history().clear();
        doc.setModified(false);
        QImage mask(4, 4, QImage::Format_Grayscale8);
        mask.fill(0);
        mask.scanLine(1)[1] = 255;
        mask.scanLine(1)[2] = 128;
        doc.selection().setMaskImage(mask);
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        auto operation = [](const QImage &image) {
            Curves curves;
            curves.setControlPoints({{0, 255}, {255, 255}});
            return curves.apply(image);
        };
        {
            AdjustmentPreview preview(&doc, &canvas, {0, 1});
            QImage displayed;
            QObject::connect(&preview, &AdjustmentPreview::previewReady, &canvas,
                             [&](const QImage &image) { displayed = image; });
            preview.request(operation);
            QElapsedTimer timer;
            timer.start();
            while (displayed.isNull() && timer.elapsed() < 5000) {
                QCoreApplication::processEvents();
                QThread::msleep(1);
            }
            CHECK(!displayed.isNull(), "Curves delivers a canvas preview");
            CHECK(doc.layerAt(0)->image() == source && doc.layerAt(1)->image() == source,
                  "Preview leaves original layer images untouched");
            preview.cancel();
            CHECK(doc.history().currentIndex() == 0 && !doc.isModified(),
                  "Cancel leaves history and modified state untouched");
        }
        AdjustmentPreview preview(&doc, &canvas, {0, 1});
        CHECK(preview.commit(operation, "Curves"), "Curves commits adjusted layers");
        const QImage after0 = doc.layerAt(0)->image();
        const QImage after1 = doc.layerAt(1)->image();
        CHECK(after0.format() == QImage::Format_ARGB32_Premultiplied &&
              after1.format() == QImage::Format_ARGB32_Premultiplied,
              "Curves preserves premultiplied layer storage");
        CHECK(after0.pixel(1, 1) == qRgba(128, 128, 128, 128),
              "Curves maps straight colour then restores premultiplication");
        CHECK(after0.pixel(2, 1) == qRgba(80, 88, 96, 128),
              "Feathered selection blends premultiplied colour without changing alpha");
        CHECK(after1.pixel(0, 1) == after0.pixel(1, 1),
              "Selection follows document coordinates on an offset layer");
        CHECK(after0.pixel(3, 3) == source.pixel(3, 3) && after0.pixel(0, 0) == 0,
              "Unselected and transparent pixels remain unchanged");
        CHECK(doc.history().currentIndex() == 1, "Multiple layers create one Curves undo entry");
        doc.history().undo();
        CHECK(doc.layerAt(0)->image() == source && doc.layerAt(1)->image() == source,
              "Undo restores both original layers");
        doc.history().redo();
        CHECK(doc.layerAt(0)->image() == after0 && doc.layerAt(1)->image() == after1,
              "Redo restores both adjusted layers");
    }

    SECTION("Brightness/Contrast dialog preserves thumbnail and reports parameter changes");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 128, 128));
        PreviewDialog dialog("Brightness / Contrast", source,
            {{"Brightness", -100, 100, 0, ""}, {"Contrast", -100, 100, 0, ""}},
            [](const QImage &image, const QVector<int> &values) {
                BrightnessContrast adjustment;
                adjustment.setBrightness(values[0]);
                adjustment.setContrast(values[1]);
                return adjustment.apply(image);
            });
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr, "Existing thumbnail remains available");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        dialog.findChildren<QSlider *>().first()->setValue(40);
        CHECK(changes == 1 && dialog.values()[0] == 40, "Slider emits one synchronised parameter notification");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Thumbnail still updates on slider change");
        dialog.findChildren<QSpinBox *>().last()->setValue(20);
        CHECK(changes == 2 && dialog.values()[1] == 20, "Numeric entry emits synchronised parameter notification");
    }

    SECTION("Hue/Saturation control notifications and neutral saturation");
    {
        HueSaturationDialog dialog;
        int changes = 0;
        QObject::connect(&dialog, &HueSaturationDialog::parametersChanged, &dialog, [&]() { ++changes; });
        CHECK(dialog.hue() == 0 && dialog.saturation() == 100 && dialog.lightness() == 0,
              "Hue/Saturation starts with neutral controls");
        HueSaturation adjustment;
        adjustment.setSaturation(dialog.saturation() - 100);
        QImage source = makeImage(8, 8, QColor(64, 96, 128, 128));
        CHECK(adjustment.apply(source).convertToFormat(source.format()) == source,
              "Neutral dialog saturation maps to an unchanged image");
        dialog.findChildren<QSlider *>().first()->setValue(30);
        CHECK(changes == 1 && dialog.hue() == 30, "Hue slider reports parameter changes");
        dialog.findChildren<QSpinBox *>()[1]->setValue(140);
        CHECK(changes == 2 && dialog.saturation() == 140, "Saturation numeric entry reports changes");
        dialog.findChildren<QSpinBox *>().last()->setValue(20);
        CHECK(changes == 3 && dialog.lightness() == 20, "Lightness numeric entry reports changes");
        for (auto *button : dialog.findChildren<QPushButton *>())
            if (button->text() == "↺") button->click();
        CHECK(dialog.hue() == 0 && dialog.saturation() == 100 && dialog.lightness() == 0,
              "Per-control Reset restores neutral Hue/Saturation values");
    }

    SECTION("Levels thumbnail, gamma controls and neutral mapping");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 128, 128));
        PreviewDialog dialog("Levels", source,
            {{"Input black", 0, 255, 0, ""}, {"Input white", 0, 255, 255, ""},
             {"Gamma", 10, 1000, 100, "%"}},
            [](const QImage &image, const QVector<int> &values) {
                Levels adjustment;
                adjustment.setInputBlack(values[0]);
                adjustment.setInputWhite(values[1]);
                adjustment.setGamma(values[2] / 100.0);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr, "Levels retains its thumbnail");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>().last()->setValue(180);
        CHECK(changes == 1 && dialog.values()[2] == 180, "Gamma numeric entry notifies canvas preview");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Gamma updates existing thumbnail");
        Levels neutral;
        CHECK(neutral.apply(source).convertToFormat(source.format()) == source,
              "Default Levels preserves neutral pixels and alpha");
    }

    SECTION("Sepia thumbnail, intensity and alpha preservation");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 128, 128));
        source.setPixel(0, 0, 0);
        PreviewDialog dialog("Sepia", source, {{"Intensity", 0, 100, 80, "%"}},
            [](const QImage &image, const QVector<int> &values) {
                Sepia adjustment;
                adjustment.setIntensity(values[0]);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr && dialog.values()[0] == 80, "Sepia retains thumbnail and default intensity");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>().first()->setValue(0);
        CHECK(changes == 1 && dialog.values()[0] == 0, "Sepia numeric entry notifies preview");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Sepia thumbnail updates with intensity");
        Sepia adjustment;
        adjustment.setIntensity(0);
        CHECK(adjustment.apply(source).convertToFormat(source.format()) == source,
              "Zero Sepia intensity preserves original pixels");
        adjustment.setIntensity(100);
        const QImage processed = adjustment.apply(source).convertToFormat(source.format());
        CHECK(qAlpha(processed.pixel(1, 1)) == 128 && processed.pixel(0, 0) == 0,
              "Sepia preserves partial alpha and transparent pixels");
    }

    SECTION("Posterize thumbnail, level controls and alpha preservation");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 192, 128));
        source.setPixel(0, 0, 0);
        PreviewDialog dialog("Posterize", source, {{"Levels", 2, 64, 4, ""}},
            [](const QImage &image, const QVector<int> &values) {
                Posterize adjustment;
                adjustment.setLevels(values[0]);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr && dialog.values()[0] == 4, "Posterize retains thumbnail and default four levels");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>().first()->setValue(2);
        CHECK(changes == 1 && dialog.values()[0] == 2, "Posterize numeric entry notifies preview");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Posterize thumbnail updates with levels");
        dialog.findChildren<QSlider *>().first()->setValue(64);
        CHECK(changes == 2 && dialog.values()[0] == 64, "Posterize maximum levels synchronise controls");
        Posterize adjustment;
        adjustment.setLevels(2);
        const QImage processed = adjustment.apply(source);
        CHECK(qRed(processed.pixel(1, 1)) == 0 && qBlue(processed.pixel(1, 1)) == 255,
              "Posterize quantises straight colour channels at two levels");
        const QImage stored = processed.convertToFormat(source.format());
        CHECK(qAlpha(stored.pixel(1, 1)) == 128 && stored.pixel(0, 0) == 0,
              "Posterize preserves partial alpha and transparent pixels");
    }

    SECTION("Threshold thumbnail, control endpoints and alpha preservation");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 192, 128));
        source.setPixel(0, 0, 0);
        PreviewDialog dialog("Threshold", source, {{"Threshold", 0, 255, 128, ""}},
            [](const QImage &image, const QVector<int> &values) {
                Threshold adjustment;
                adjustment.setThreshold(values[0]);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr && dialog.values()[0] == 128, "Threshold retains thumbnail and default 128");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>().first()->setValue(0);
        CHECK(changes == 1 && dialog.values()[0] == 0, "Threshold numeric entry notifies preview");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Threshold thumbnail updates with control changes");
        dialog.findChildren<QSlider *>().first()->setValue(255);
        CHECK(changes == 2 && dialog.values()[0] == 255, "Threshold upper endpoint synchronises controls");
        Threshold adjustment;
        adjustment.setThreshold(0);
        const QImage white = adjustment.apply(source).convertToFormat(source.format());
        CHECK(white.pixel(1, 1) == qRgba(128, 128, 128, 128) && white.pixel(0, 0) == 0,
              "Threshold zero produces white while preserving partial and zero alpha");
        adjustment.setThreshold(255);
        const QImage black = adjustment.apply(source).convertToFormat(source.format());
        CHECK(black.pixel(1, 1) == qRgba(0, 0, 0, 128) && black.pixel(0, 0) == 0,
              "Threshold 255 produces black for darker pixels while preserving alpha");
    }

    SECTION("Color Balance thumbnail, channel controls and alpha preservation");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 192, 128));
        source.setPixel(0, 0, 0);
        PreviewDialog dialog("Color Balance", source,
            {{"Cyan / Red", -100, 100, 0, ""}, {"Magenta / Green", -100, 100, 0, ""},
             {"Yellow / Blue", -100, 100, 0, ""}},
            [](const QImage &image, const QVector<int> &values) {
                ColorBalance adjustment;
                adjustment.setCyanRed(values[0]);
                adjustment.setMagentaGreen(values[1]);
                adjustment.setYellowBlue(values[2]);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr && dialog.values() == QVector<int>({0,0,0}),
              "Color Balance retains thumbnail and neutral defaults");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>()[0]->setValue(30);
        dialog.findChildren<QSlider *>()[1]->setValue(-20);
        dialog.findChildren<QSpinBox *>()[2]->setValue(40);
        CHECK(changes == 3 && dialog.values() == QVector<int>({30,-20,40}),
              "All Color Balance channels notify preview with synchronised values");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Color Balance thumbnail updates");
        ColorBalance adjustment;
        CHECK(adjustment.apply(source).convertToFormat(source.format()) == source,
              "Neutral Color Balance preserves original pixels");
        adjustment.setCyanRed(30); adjustment.setMagentaGreen(-20); adjustment.setYellowBlue(40);
        const QImage processed = adjustment.apply(source).convertToFormat(source.format());
        CHECK(qAlpha(processed.pixel(1, 1)) == 128 && processed.pixel(0, 0) == 0,
              "Color Balance preserves partial alpha and transparent pixels");
    }

    SECTION("Exposure thumbnail, signed controls and alpha preservation");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 192, 128));
        source.setPixel(0, 0, 0);
        PreviewDialog dialog("Exposure", source, {{"Exposure", -100, 100, 0, ""}},
            [](const QImage &image, const QVector<int> &values) {
                Exposure adjustment;
                adjustment.setExposure(values[0]);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr && dialog.values()[0] == 0, "Exposure retains thumbnail and neutral default");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>().first()->setValue(-50);
        CHECK(changes == 1 && dialog.values()[0] == -50, "Negative Exposure numeric entry notifies preview");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Exposure thumbnail updates");
        dialog.findChildren<QSlider *>().first()->setValue(50);
        CHECK(changes == 2 && dialog.values()[0] == 50, "Positive Exposure slider synchronises controls");
        Exposure adjustment;
        CHECK(adjustment.apply(source).convertToFormat(source.format()) == source,
              "Neutral Exposure preserves original pixels");
        adjustment.setExposure(50);
        const QImage bright = adjustment.apply(source).convertToFormat(source.format());
        adjustment.setExposure(-50);
        const QImage dark = adjustment.apply(source).convertToFormat(source.format());
        CHECK(qRed(bright.pixel(1, 1)) > qRed(source.pixel(1, 1)) &&
              qRed(dark.pixel(1, 1)) < qRed(source.pixel(1, 1)),
              "Exposure brightens and darkens relative to original");
        CHECK(qAlpha(bright.pixel(1, 1)) == 128 && qAlpha(dark.pixel(1, 1)) == 128 &&
              bright.pixel(0, 0) == 0 && dark.pixel(0, 0) == 0,
              "Exposure preserves partial alpha and transparent pixels");
    }

    SECTION("Highlights/Shadows thumbnail, controls and alpha preservation");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 192, 128));
        source.setPixel(0, 0, 0);
        PreviewDialog dialog("Highlights / Shadows", source,
            {{"Highlights", -100, 100, 0, ""}, {"Shadows", -100, 100, 0, ""}},
            [](const QImage &image, const QVector<int> &values) {
                HighlightsShadows adjustment;
                adjustment.setHighlights(values[0]);
                adjustment.setShadows(values[1]);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr && dialog.values() == QVector<int>({0,0}),
              "Highlights/Shadows retains thumbnail and neutral defaults");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>().first()->setValue(-40);
        dialog.findChildren<QSlider *>().last()->setValue(60);
        CHECK(changes == 2 && dialog.values() == QVector<int>({-40,60}),
              "Both Highlights/Shadows controls notify preview with synchronised values");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Highlights/Shadows thumbnail updates");
        HighlightsShadows adjustment;
        CHECK(adjustment.apply(source).convertToFormat(source.format()) == source,
              "Neutral Highlights/Shadows preserves original pixels");
        adjustment.setHighlights(-40);
        const QImage dark = adjustment.apply(source).convertToFormat(source.format());
        adjustment.setHighlights(0); adjustment.setShadows(60);
        const QImage bright = adjustment.apply(source).convertToFormat(source.format());
        CHECK(qRed(dark.pixel(1, 1)) < qRed(source.pixel(1, 1)) &&
              qRed(bright.pixel(1, 1)) > qRed(source.pixel(1, 1)),
              "Highlights and shadows independently affect the original image");
        CHECK(qAlpha(bright.pixel(1, 1)) == 128 && qAlpha(dark.pixel(1, 1)) == 128 &&
              bright.pixel(0, 0) == 0 && dark.pixel(0, 0) == 0,
              "Highlights/Shadows preserves partial alpha and transparent pixels");
    }

    SECTION("Temperature/Tint thumbnail, controls and alpha preservation");
    {
        QImage source = makeImage(8, 8, QColor(64, 96, 192, 128));
        source.setPixel(0, 0, 0);
        PreviewDialog dialog("Temperature / Tint", source,
            {{"Temperature", -100, 100, 0, ""}, {"Tint", -100, 100, 0, ""}},
            [](const QImage &image, const QVector<int> &values) {
                TemperatureTint adjustment;
                adjustment.setTemperature(values[0]);
                adjustment.setTint(values[1]);
                return adjustment.apply(image);
            });
        QLabel *thumbnail = nullptr;
        for (auto *label : dialog.findChildren<QLabel *>())
            if (!label->pixmap().isNull()) thumbnail = label;
        CHECK(thumbnail != nullptr && dialog.values() == QVector<int>({0,0}),
              "Temperature/Tint retains thumbnail and neutral defaults");
        const QImage before = thumbnail ? thumbnail->pixmap().toImage() : QImage();
        int changes = 0;
        QObject::connect(&dialog, &PreviewDialog::parametersChanged, &dialog, [&]() { ++changes; });
        dialog.findChildren<QSpinBox *>().first()->setValue(-40);
        dialog.findChildren<QSlider *>().last()->setValue(60);
        CHECK(changes == 2 && dialog.values() == QVector<int>({-40,60}),
              "Both Temperature/Tint controls notify preview with synchronised values");
        CHECK(thumbnail && thumbnail->pixmap().toImage() != before, "Temperature/Tint thumbnail updates");
        TemperatureTint adjustment;
        CHECK(adjustment.apply(source).convertToFormat(source.format()) == source,
              "Neutral Temperature/Tint preserves original pixels");
        adjustment.setTemperature(50);
        const QImage warm = adjustment.apply(source).convertToFormat(source.format());
        adjustment.setTemperature(-50);
        const QImage cool = adjustment.apply(source).convertToFormat(source.format());
        CHECK(qRed(warm.pixel(1, 1)) > qRed(source.pixel(1, 1)) &&
              qBlue(warm.pixel(1, 1)) < qBlue(source.pixel(1, 1)) &&
              qRed(cool.pixel(1, 1)) < qRed(source.pixel(1, 1)) &&
              qBlue(cool.pixel(1, 1)) > qBlue(source.pixel(1, 1)),
              "Positive and negative temperature shift red and blue in opposite directions");
        adjustment.setTemperature(0); adjustment.setTint(50);
        const QImage tinted = adjustment.apply(source).convertToFormat(source.format());
        CHECK(qGreen(tinted.pixel(1, 1)) < qGreen(source.pixel(1, 1)), "Positive tint reduces green");
        CHECK(qAlpha(warm.pixel(1, 1)) == 128 && qAlpha(cool.pixel(1, 1)) == 128 &&
              qAlpha(tinted.pixel(1, 1)) == 128 && warm.pixel(0, 0) == 0 &&
              cool.pixel(0, 0) == 0 && tinted.pixel(0, 0) == 0,
              "Temperature/Tint preserves partial alpha and transparent pixels");
    }

    // ---------- I18N ----------
    SECTION("Internationalisation");
    {
        I18n::setLanguage(I18n::Lang::French);
        CHECK(I18n::t("&Fichier") == "&Fichier", "French returns source string");
        I18n::setLanguage(I18n::Lang::English);
        CHECK(I18n::t("&Fichier") == "&File", "English translates File menu");
        CHECK(I18n::t("Pinceau") == "Paintbrush", "English translates Paintbrush");
        CHECK(I18n::t("Sombre") == "Dark", "English translates Dark");
        CHECK(I18n::t("ZZ-unknown-ZZ") == "ZZ-unknown-ZZ", "Unknown string passes through");
        I18n::setLanguage(I18n::Lang::French);
    }

    // ---------- THEME ----------
    SECTION("Theming");
    {
        Theme::setScheme(Theme::Scheme::Light);
        CHECK(!Theme::isDark(), "Light scheme is not dark");
        QString ls = Theme::styleSheet();
        CHECK(ls.contains("QMenuBar"), "Light stylesheet has content");
        Theme::setScheme(Theme::Scheme::Dark);
        CHECK(Theme::isDark(), "Dark scheme is dark");
        QString ds = Theme::styleSheet();
        CHECK(ds.contains("QMenuBar") && ds != ls, "Dark stylesheet differs from light");
        CHECK(Theme::canvasBackdrop() != QString("#969696"), "Dark backdrop differs from light");
        Theme::setScheme(Theme::Scheme::Light);
    }

    // ---------- DOCUMENT RESIZE / CROP ----------
    SECTION("Resize / canvas size / crop");
    {
        Document doc(40, 30);
        doc.resize(80, 60);
        CHECK(doc.width()==80 && doc.height()==60, "resize scales the document");
        CHECK(doc.history().canUndo(), "resize is undoable");
        Document d2(40, 30);
        d2.resizeCanvas(60, 60, 10, 10);
        CHECK(d2.width()==60 && d2.height()==60, "canvas size changes dimensions");
        Document d3(40, 40);
        d3.cropTo(QRect(5, 5, 20, 20));
        CHECK(d3.width()==20 && d3.height()==20, "crop resizes the document");
        CHECK(d3.history().canUndo(), "crop is undoable");
    }

    // ---------- SELECTION TOOLS ----------
    SECTION("Selection tools create selections");
    {
        // Rectangle select
        {
            Document doc(64,64); CanvasWidget canvas; canvas.setDocument(&doc);
            SelectionTool rect(SelectionShape::Rectangle);
            strokeTool(&rect, canvas, QPointF(10,10), QPointF(40,40));
            CHECK(doc.selection().hasSelection(), "Rectangle select creates a selection");
            CHECK(doc.selection().isSelected(25,25), "Rectangle selection covers dragged area");
            CHECK(!doc.selection().isSelected(55,55), "Rectangle selection excludes outside");
        }
        // Ellipse select
        {
            Document doc(64,64); CanvasWidget canvas; canvas.setDocument(&doc);
            SelectionTool ell(SelectionShape::Ellipse);
            strokeTool(&ell, canvas, QPointF(5,5), QPointF(60,60));
            CHECK(doc.selection().hasSelection(), "Ellipse select creates a selection");
            CHECK(doc.selection().isSelected(32,32), "Ellipse selection covers centre");
        }
        // Lasso
        {
            Document doc(64,64); CanvasWidget canvas; canvas.setDocument(&doc);
            LassoTool lasso;
            QMouseEvent e1 = pressEv(QPointF(10,10)); lasso.mousePressEvent(QPointF(10,10), &e1, canvas);
            for (QPointF p : {QPointF(50,10), QPointF(50,50), QPointF(10,50)}) {
                QMouseEvent m = moveEv(p); lasso.mouseMoveEvent(p, &m, canvas);
            }
            QMouseEvent r = releaseEv(QPointF(10,10)); lasso.mouseReleaseEvent(QPointF(10,10), &r, canvas);
            CHECK(doc.selection().hasSelection(), "Lasso creates a selection");
        }
        // Magic wand (contiguous region of same colour)
        {
            Document doc(64,64); doc.activeLayer()->clear(Qt::white);
            { QPainter p(&doc.activeLayer()->image()); p.fillRect(0,0,32,64,Qt::red); p.end(); }
            CanvasWidget canvas; canvas.setDocument(&doc);
            MagicWandTool wand; wand.setTolerance(20);
            QMouseEvent e = pressEv(QPointF(10,10)); wand.mousePressEvent(QPointF(10,10), &e, canvas);
            QMouseEvent r = releaseEv(QPointF(10,10)); wand.mouseReleaseEvent(QPointF(10,10), &r, canvas);
            CHECK(doc.selection().hasSelection(), "Magic wand creates a selection");
            CHECK(doc.selection().isSelected(10,10) && !doc.selection().isSelected(50,10),
                  "Magic wand selects only the same-colour region");
        }
    }

    // ---------- MOVE TOOL ----------
    SECTION("Move tool");
    {
        Document doc(64,64); doc.activeLayer()->clear(Qt::transparent);
        { QPainter p(&doc.activeLayer()->image()); p.fillRect(0,0,20,20, QColor(255,0,0,255)); p.end(); }
        CanvasWidget canvas; canvas.setDocument(&doc);
        QColor origin = doc.activeLayer()->image().pixelColor(5,5);   // red
        MoveTool move;
        strokeTool(&move, canvas, QPointF(5,5), QPointF(35,35));   // shift by +30,+30
        QImage &img = doc.activeLayer()->image();
        CHECK(origin.red()>200, "content present before move");
        CHECK(img.pixelColor(35,35).red()>200, "Move tool relocated the pixels");
        CHECK(doc.history().canUndo(), "Move is undoable");
    }

    // ---------- TEXT TOOL ----------
    SECTION("Text tool");
    {
        Document doc(80,40); doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::black);
        CanvasWidget canvas; canvas.setDocument(&doc);
        TextTool text;
        QMouseEvent e = pressEv(QPointF(5,20)); text.mousePressEvent(QPointF(5,20), &e, canvas);
        for (QChar ch : QString("Hi")) {
            QKeyEvent k(QEvent::KeyPress, 0, Qt::NoModifier, QString(ch));
            text.keyPressEvent(&k, canvas);
        }
        QImage before = doc.activeLayer()->image().copy();
        QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        text.keyPressEvent(&enter, canvas);   // commit
        CHECK(imagesDiffer(before, doc.activeLayer()->image()), "Text tool commits text to the layer");
    }

    // ---------- COLOR PICKER ----------
    SECTION("Color picker");
    {
        Document doc(32,32); doc.activeLayer()->clear(QColor(10,200,40));
        doc.setPrimaryColor(Qt::black);
        CanvasWidget canvas; canvas.setDocument(&doc);
        ColorPickerTool picker;
        QMouseEvent e = pressEv(QPointF(16,16)); picker.mousePressEvent(QPointF(16,16), &e, canvas);
        QColor picked = doc.primaryColor();
        CHECK(picked.green()>150 && picked.red()<80, "Color picker samples the pixel under the cursor");
    }

    // ---------- COLOR WHEEL LOGIC (the reported bug) ----------
    SECTION("Color wheel picks a visible colour");
    {
        ColorWheelWidget wheel;
        wheel.setColor(Qt::black);          // value = 0 (the default)
        wheel.resize(200, 200);
        QPixmap pm(200,200);
        wheel.render(&pm);                  // forces paintEvent -> sets wheelRadius
        QColor got = Qt::black;
        QObject::connect(&wheel, &ColorWheelWidget::colorChanged,
                         [&](const QColor &c){ got = c; });
        // Click clearly inside the disc, to the right of centre (red-ish hue).
        QPointF pos(150, 100);
        QMouseEvent press(QEvent::MouseButtonPress, pos, wheel.mapToGlobal(pos.toPoint()),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&wheel, &press);
        CHECK(got.value() > 10, "Picking on the wheel does NOT stay black (value lifted)");
        CHECK(got.saturation() > 10, "Picking on the wheel gives a saturated colour");
    }

    // ================================================================
    // BEHAVIOURAL / INTEGRATION TESTS (what the user actually observes)
    // ================================================================

    // ---------- LAYER OPACITY REVEALS THE LAYER BEHIND ----------
    SECTION("Reducing a layer's opacity reveals the layer behind");
    {
        Document doc(32,32);
        doc.activeLayer()->clear(QColor(0,0,255));           // bottom = blue
        int top = doc.addLayer();
        doc.layerAt(top)->clear(QColor(255,0,0));            // top = red, opaque
        // Fully opaque: result is pure red.
        QColor opaque = doc.flatten().pixelColor(16,16);
        CHECK(opaque.red()>230 && opaque.blue()<40, "Opaque top layer hides the one below (red)");
        // Half opacity: red blends with the blue behind -> both channels present.
        doc.setLayerOpacity(top, 0.5f);
        QColor half = doc.flatten().pixelColor(16,16);
        CHECK(half.red()>60 && half.blue()>60,
              "At 50% opacity you SEE the blue layer through the red one");
        // Fully transparent top: result is pure blue (the layer behind).
        doc.setLayerOpacity(top, 0.0f);
        QColor clear = doc.flatten().pixelColor(16,16);
        CHECK(clear.blue()>230 && clear.red()<40,
              "At 0% opacity only the layer behind (blue) shows");
    }

    // ---------- HIDING A LAYER REVEALS THE ONE BEHIND ----------
    SECTION("Toggling layer visibility");
    {
        Document doc(16,16);
        doc.activeLayer()->clear(QColor(0,180,0));    // bottom green
        int top = doc.addLayer();
        doc.layerAt(top)->clear(QColor(200,0,0));     // top red
        CHECK(doc.flatten().pixelColor(8,8).red()>180, "top layer visible -> red");
        doc.setLayerVisibility(top, false);
        CHECK(doc.flatten().pixelColor(8,8).green()>150, "top hidden -> green shows through");
    }

    // ---------- BRUSH SIZE CHANGES THE FOOTPRINT ----------
    SECTION("Brush size actually changes the stroke width");
    {
        auto paintedPixels = [](int size) {
            Document doc(80,80); doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::black);
            CanvasWidget canvas; canvas.setDocument(&doc);
            BrushTool brush; brush.setBrushSize(size); brush.setHardness(100);
            QMouseEvent e = pressEv(QPointF(40,40));
            brush.mousePressEvent(QPointF(40,40), &e, canvas);
            QMouseEvent r = releaseEv(QPointF(40,40));
            brush.mouseReleaseEvent(QPointF(40,40), &r, canvas);
            return countNonBackground(doc.activeLayer()->image(), QColor(Qt::white));
        };
        int small = paintedPixels(4);
        int big = paintedPixels(40);
        CHECK(small > 0, "small brush paints something");
        CHECK(big > small * 4, "a bigger brush paints a much larger area");
    }

    // ---------- ERASER SIZE CHANGES THE FOOTPRINT ----------
    SECTION("Eraser size changes the erased area");
    {
        auto erasedPixels = [](int size) {
            Document doc(80,80); doc.activeLayer()->clear(QColor(0,0,0,255));
            CanvasWidget canvas; canvas.setDocument(&doc);
            EraserTool er; er.setBrushSize(size); er.setHardness(100);
            QMouseEvent e = pressEv(QPointF(40,40));
            er.mousePressEvent(QPointF(40,40), &e, canvas);
            QMouseEvent r = releaseEv(QPointF(40,40));
            er.mouseReleaseEvent(QPointF(40,40), &r, canvas);
            QImage &img = doc.activeLayer()->image();
            int transparent = 0;
            for (int y=0;y<80;++y) for (int x=0;x<80;++x)
                if (img.pixelColor(x,y).alpha() < 128) ++transparent;
            return transparent;
        };
        CHECK(erasedPixels(40) > erasedPixels(6) * 4, "a bigger eraser clears a much larger area");
    }

    // ---------- SELECTION SELECTS THE RIGHT ZONE ----------
    SECTION("Selecting a zone then filling only affects that zone");
    {
        Document doc(64,64); doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::red);
        // Select a precise rectangle.
        doc.selection().selectRect(QRect(16,16,16,16), SelectionMode::Replace);
        CanvasWidget canvas; canvas.setDocument(&doc);
        FillTool fill; fill.setTolerance(255);
        QMouseEvent e = pressEv(QPointF(24,24));
        fill.mousePressEvent(QPointF(24,24), &e, canvas);
        QMouseEvent r = releaseEv(QPointF(24,24));
        fill.mouseReleaseEvent(QPointF(24,24), &r, canvas);
        QImage &img = doc.activeLayer()->image();
        // Corners of the selected rect should be red; everything else white.
        CHECK(img.pixelColor(20,20).red()>200 && img.pixelColor(20,20).blue()<80,
              "fill covers the selected zone");
        CHECK(img.pixelColor(2,2)==QColor(Qt::white) && img.pixelColor(60,60)==QColor(Qt::white),
              "fill leaves everything OUTSIDE the selection untouched");
        // Precise boundary: (31,31) inside, (33,33) outside.
        CHECK(img.pixelColor(31,31).red()>200, "boundary pixel inside selection is filled");
        CHECK(img.pixelColor(33,33)==QColor(Qt::white), "boundary pixel outside selection is clean");
    }

    // ---------- MAKE AN IMPORTED IMAGE PARTLY TRANSPARENT ----------
    SECTION("Import an image as a layer and make it transparent over another");
    {
        // Background document (a solid photo-like layer).
        Document doc(40,40); doc.activeLayer()->clear(QColor(30,140,30));  // green bg
        // "Import" a second image as a new layer.
        QImage imported = makeImage(40,40, QColor(200,40,40));             // red image
        int idx = doc.addLayer(imported, "Imported");
        CHECK(doc.layerCount()==2, "imported image becomes a new layer");
        CHECK(doc.flatten().pixelColor(20,20).red()>180, "imported image on top (red)");
        // Make it half transparent -> green shows through.
        doc.setLayerOpacity(idx, 0.4f);
        QColor blended = doc.flatten().pixelColor(20,20);
        CHECK(blended.green()>60 && blended.red()>60,
              "transparent imported image lets the background show through");
    }

    // ---------- MULTI-DOCUMENT INDEPENDENCE ----------
    SECTION("Multiple open images are independent");
    {
        Document a(20,20); a.activeLayer()->clear(Qt::red);
        Document b(30,30); b.activeLayer()->clear(Qt::blue);
        CHECK(a.width()==20 && b.width()==30, "two documents keep their own size");
        CHECK(a.flatten().pixelColor(5,5).red()>200, "doc A is red");
        CHECK(b.flatten().pixelColor(5,5).blue()>200, "doc B is blue");
        // Editing A must not touch B.
        a.activeLayer()->clear(Qt::black);
        CHECK(b.flatten().pixelColor(5,5).blue()>200, "editing A leaves B unchanged");
    }

    // ---------- TOLERANCE AFFECTS FILL SPREAD ----------
    SECTION("Fill tolerance controls the flooded area");
    {
        auto filled = [](int tol) {
            Document doc(40,40);
            // gradient-ish: left half 100, right half 130 grey
            QImage img = makeImage(40,40, QColor(100,100,100,255));
            { QPainter p(&img); p.fillRect(20,0,20,40, QColor(130,130,130)); p.end(); }
            doc.activeLayer()->setImage(img);
            doc.setPrimaryColor(Qt::red);
            CanvasWidget canvas; canvas.setDocument(&doc);
            FillTool fill; fill.setTolerance(tol);
            QMouseEvent e = pressEv(QPointF(5,20));
            fill.mousePressEvent(QPointF(5,20), &e, canvas);
            QMouseEvent r = releaseEv(QPointF(5,20));
            fill.mouseReleaseEvent(QPointF(5,20), &r, canvas);
            int red = 0; QImage &out = doc.activeLayer()->image();
            for (int y=0;y<40;++y) for (int x=0;x<40;++x)
                if (out.pixelColor(x,y).red()>200) ++red;
            return red;
        };
        int low = filled(10);    // only the left half (~800 px)
        int high = filled(200);  // crosses into the right half (~1600 px)
        CHECK(low > 100 && low < 1200, "low tolerance stays within the similar region");
        CHECK(high > low, "higher tolerance floods a larger area");
    }

    // ---------- SAMPLING: Image vs Layer (Paint.NET) ----------
    SECTION("Magic Wand sampling: Layer selects differently than Image");
    {
        // Bottom layer: fully red. Top (active) layer: left half red, right half
        // transparent. Composite (Image) is all-red; the top layer alone (Layer)
        // has a transparent right half.
        Document doc(32, 32);
        doc.activeLayer()->clear(Qt::red);                       // bottom layer, index 0
        QImage top = makeImage(32, 32, QColor(255, 0, 0, 255));  // red
        { QPainter p(&top); p.setCompositionMode(QPainter::CompositionMode_Source);
          p.fillRect(16, 0, 16, 32, Qt::transparent); p.end(); }
        int topIdx = doc.addLayer(top, "Top");
        doc.setActiveLayer(topIdx);
        CanvasWidget canvas; canvas.setDocument(&doc);

        // Sampling = Image: the composite is uniform red, so the right side is
        // part of the same-colour region.
        {
            MagicWandTool wand; wand.setTolerance(20); wand.setSampleImage(true);
            QMouseEvent e = pressEv(QPointF(2, 2)); wand.mousePressEvent(QPointF(2, 2), &e, canvas);
            CHECK(doc.selection().isSelected(24, 16),
                  "Sampling=Image: red shows through in the composite, right side selected");
        }
        // Sampling = Layer: the active layer's right half is transparent, so it is
        // NOT part of the red region.
        {
            MagicWandTool wand; wand.setTolerance(20); wand.setSampleImage(false);
            QMouseEvent e = pressEv(QPointF(2, 2)); wand.mousePressEvent(QPointF(2, 2), &e, canvas);
            CHECK(doc.selection().isSelected(2, 2),
                  "Sampling=Layer: the clicked red pixel is selected");
            CHECK(!doc.selection().isSelected(24, 16),
                  "Sampling=Layer: the transparent right half is excluded");
        }
    }

    SECTION("Paint Bucket sampling: Image decides region, fill writes active layer");
    {
        // Bottom layer: left half blue, right half green (opaque). Top (active)
        // layer: fully transparent. With Sampling=Image the region is decided from
        // the composite (contiguous blue = left half), but the paint lands on the
        // active (top) layer.
        Document doc(32, 32);
        QImage bottom = makeImage(32, 32, QColor(0, 0, 255, 255));  // blue
        { QPainter p(&bottom); p.fillRect(16, 0, 16, 32, QColor(0, 255, 0, 255)); p.end(); }
        doc.activeLayer()->setImage(bottom);                       // index 0
        int topIdx = doc.addLayer(makeImage(32, 32, Qt::transparent), "Top");
        doc.setActiveLayer(topIdx);
        doc.setPrimaryColor(Qt::red);
        CanvasWidget canvas; canvas.setDocument(&doc);

        FillTool fill; fill.setTolerance(20); fill.setSampleImage(true);
        QMouseEvent e = pressEv(QPointF(4, 4)); fill.mousePressEvent(QPointF(4, 4), &e, canvas);

        QImage &topImg = doc.activeLayer()->image();
        CHECK(topImg.pixelColor(4, 4).red() > 200 && topImg.pixelColor(4, 4).alpha() > 200,
              "Sampling=Image: composite's blue region on the left is filled red on the active layer");
        CHECK(topImg.pixelColor(28, 4).alpha() < 40,
              "Sampling=Image: the green (right) region is not part of the flood, stays transparent");
        // The paint must go to the ACTIVE (top) layer, never the sampled bottom.
        CHECK(doc.layerAt(0)->image().pixelColor(4, 4).blue() > 200,
              "the bottom (sampled) layer is left untouched by the fill");
    }

    // ---------- SMART MERGE (keep both drawings) ----------
    SECTION("Merge keeps both drawings when top has a flat background");
    {
        Document doc(40, 40);
        doc.activeLayer()->clear(Qt::white);
        { QPainter p(&doc.activeLayer()->image()); p.fillRect(2,2,12,12, QColor(0,0,255)); p.end(); } // bottom: blue square
        int top = doc.addLayer();            // white layer
        doc.layerAt(top)->clear(Qt::white);
        { QPainter p(&doc.layerAt(top)->image()); p.fillRect(26,26,12,12, QColor(255,0,0)); p.end(); } // top: red square
        doc.mergeLayerDown(top, true);       // keepArtwork
        CHECK(doc.layerCount() == 1, "merge collapses to one layer");
        QImage flat = doc.flatten();
        CHECK(flat.pixelColor(8,8).blue() > 200 && flat.pixelColor(8,8).red() < 80,
              "bottom drawing (blue) survives the merge");
        CHECK(flat.pixelColor(32,32).red() > 200 && flat.pixelColor(32,32).blue() < 80,
              "top drawing (red) survives the merge");
    }

    // ---------- REGRESSION: audit bug fixes ----------
    SECTION("Rotate 90° swaps non-square dimensions and keeps every layer");
    {
        Document doc(80, 40);                       // non-square
        doc.activeLayer()->clear(Qt::white);
        { QPainter p(&doc.activeLayer()->image()); p.fillRect(0,0,10,10, QColor(0,0,255)); p.end(); }
        int top = doc.addLayer();                   // second layer with red mark
        { QPainter p(&doc.layerAt(top)->image()); p.fillRect(70,30,10,10, QColor(255,0,0)); p.end(); }
        doc.rotate(1, "Rotate 90° CW");
        CHECK(doc.width() == 40 && doc.height() == 80, "document dimensions swapped");
        CHECK(doc.layerCount() == 2, "both layers preserved");
        CHECK(doc.layerAt(0)->width() == 40 && doc.layerAt(0)->height() == 80, "layer 0 resized");
        CHECK(doc.layerAt(1)->width() == 40 && doc.layerAt(1)->height() == 80, "layer 1 resized (all layers rotate)");
        doc.history().undo();
        CHECK(doc.width() == 80 && doc.height() == 40, "undo restores original dimensions");
    }

    SECTION("Flip keeps dimensions and mirrors content");
    {
        Document doc(40, 20);
        doc.activeLayer()->clear(Qt::white);
        doc.activeLayer()->image().setPixelColor(0, 0, QColor(0,0,255));   // mark top-left
        doc.flip(true, "Flip Horizontal");
        CHECK(doc.width() == 40 && doc.height() == 20, "dimensions unchanged");
        CHECK(doc.activeLayer()->image().pixelColor(39, 0).blue() > 200, "top-left moved to top-right");
    }

    SECTION("Brush Overwrite mode does not wipe the whole layer");
    {
        Document doc(40, 40);
        doc.activeLayer()->clear(QColor(0,0,255));       // solid blue
        doc.setPrimaryColor(Qt::red);
        CanvasWidget canvas; canvas.setDocument(&doc);
        BrushTool brush; brush.setBrushSize(6); brush.setBlendMode(14);   // Overwrite
        strokeTool(&brush, canvas, QPointF(10,20), QPointF(30,20));
        QImage &out = doc.activeLayer()->image();
        CHECK(out.pixelColor(2,2).blue() > 200, "untouched corner stays blue (not wiped)");
        CHECK(out.pixelColor(20,20).red() > 200, "painted area became red");
        int transparent = 0;
        for (int y=0;y<40;++y) for (int x=0;x<40;++x) if (out.pixelColor(x,y).alpha() < 10) ++transparent;
        CHECK(transparent < 40, "layer is not blanked to transparent");
    }

    SECTION("Black and White desaturates (not a 1-bit threshold)");
    {
        QImage img = makeImage(10, 10, QColor(200, 50, 50, 255));   // saturated red
        Desaturate bw;
        QImage out = bw.apply(img);
        QColor c = out.pixelColor(5,5);
        CHECK(c.red() == c.green() && c.green() == c.blue(), "output is grey");
        CHECK(c.red() > 0 && c.red() < 255, "grey keeps a mid tone (not black/white threshold)");
    }

    SECTION("Hue/Saturation leaves greys neutral");
    {
        QImage img = makeImage(8, 8, QColor(120,120,120,255));       // pure grey
        HueSaturation hs; hs.setSaturation(80);                      // crank saturation
        QColor c = hs.apply(img).pixelColor(4,4);
        CHECK(std::abs(c.red()-c.green()) < 12 && std::abs(c.green()-c.blue()) < 12,
              "grey stays roughly neutral (no red tint from hsvHue -1)");
    }

    SECTION("Flood fill stays inside the selection");
    {
        Document doc(40, 40);
        doc.activeLayer()->clear(QColor(100,100,100,255));           // uniform → tolerance can't stop it
        doc.selection().selectRect(QRect(0,0,20,40));               // left half selected
        doc.setPrimaryColor(Qt::red);
        CanvasWidget canvas; canvas.setDocument(&doc);
        FillTool fill; fill.setTolerance(255);
        QMouseEvent e = pressEv(QPointF(5,20)); fill.mousePressEvent(QPointF(5,20), &e, canvas);
        QMouseEvent r = releaseEv(QPointF(5,20)); fill.mouseReleaseEvent(QPointF(5,20), &r, canvas);
        QImage &out = doc.activeLayer()->image();
        CHECK(out.pixelColor(5,20).red() > 200, "inside selection is filled");
        CHECK(out.pixelColor(30,20).red() < 120, "outside selection is untouched (no leak)");
    }

    SECTION("Clone stamp at size 1 does not produce NaN/garbage");
    {
        Document doc(30, 30);
        doc.activeLayer()->clear(Qt::white);
        { QPainter p(&doc.activeLayer()->image()); p.fillRect(0,0,15,30, QColor(0,120,255)); p.end(); }
        CanvasWidget canvas; canvas.setDocument(&doc);
        CloneStampTool clone; clone.setBrushSize(1);
        QMouseEvent s = pressEv(QPointF(5,15), Qt::RightButton);   // set source
        clone.mousePressEvent(QPointF(5,15), &s, canvas);
        strokeTool(&clone, canvas, QPointF(20,15), QPointF(25,15));
        QColor c = doc.activeLayer()->image().pixelColor(22,15);
        CHECK(c.isValid() && c.alpha() >= 0 && c.alpha() <= 255, "produces a valid pixel (no NaN cast)");
    }

    SECTION("Native .psw format preserves layers, opacity and blend mode");
    {
        const QString path = QDir::tempPath() + "/paintsw_roundtrip.psw";
        {
            Document doc(48, 32);
            doc.activeLayer()->clear(QColor(10, 20, 30));
            doc.activeLayer()->setName("Fond");
            int top = doc.addLayer("Dessin");
            { QPainter p(&doc.layerAt(top)->image()); p.fillRect(4,4,20,20, QColor(200,60,60)); p.end(); }
            doc.layerAt(top)->setOpacity(0.5f);
            doc.layerAt(top)->setBlendMode(BlendMode::Multiply);
            doc.layerAt(top)->setVisible(false);
            CHECK(doc.saveNative(path), "saveNative succeeds");
        }
        {
            Document doc(1, 1);
            CHECK(doc.loadNative(path), "loadNative succeeds");
            CHECK(doc.width() == 48 && doc.height() == 32, "dimensions restored");
            CHECK(doc.layerCount() == 2, "both layers restored");
            CHECK(doc.layerAt(0)->name() == "Fond", "layer 0 name restored");
            CHECK(doc.layerAt(1)->name() == "Dessin", "layer 1 name restored");
            CHECK(qAbs(doc.layerAt(1)->opacity() - 0.5f) < 0.01f, "opacity restored");
            CHECK(doc.layerAt(1)->blendMode() == BlendMode::Multiply, "blend mode restored");
            CHECK(doc.layerAt(1)->isVisible() == false, "visibility restored");
            CHECK(doc.layerAt(1)->image().pixelColor(10,10).red() > 150, "pixel content restored");
        }
        QFile::remove(path);
    }

    SECTION("Move tool moves only the selected pixels, not the whole layer");
    {
        Document doc(40, 40);
        doc.activeLayer()->clear(Qt::white);
        { QPainter p(&doc.activeLayer()->image()); p.fillRect(5, 5, 10, 10, QColor(255, 0, 0)); p.end(); }
        doc.activeLayer()->image().setPixelColor(35, 35, QColor(0, 0, 255));  // unselected marker
        doc.selection().selectRect(QRect(5, 5, 10, 10));                      // select the red square

        CanvasWidget canvas; canvas.setDocument(&doc);
        MoveTool move;
        strokeTool(&move, canvas, QPointF(10, 10), QPointF(30, 10));          // drag selection +20 in x

        QImage &out = doc.activeLayer()->image();
        CHECK(out.pixelColor(30, 10).red() > 200, "selected pixels appear at the new location");
        CHECK(out.pixelColor(10, 10).alpha() < 10, "original selected area is now a transparent hole");
        CHECK(out.pixelColor(35, 35).blue() > 200, "unselected pixels elsewhere did NOT move");
        CHECK(doc.selection().boundingRect().x() >= 24 && doc.selection().boundingRect().x() <= 26,
              "the marquee followed the pixels (~+20)");
    }

    // ---------- PLUGIN SYSTEM ----------
    SECTION("Plugin effect wrapper runs a plugin process function");
    {
        PswParam prm{ "Amount", 0, 100, 100, "%" };
        PswEffect desc{ "Test Invert", "Photo", 1, &prm, &testInvertProcess, nullptr };
        PluginEffect eff(desc, std::shared_ptr<QLibrary>());   // no library needed for this test
        CHECK(eff.name() == "Test Invert", "plugin name copied out of C struct");
        CHECK(eff.paramCount() == 1, "plugin param count");

        QImage img = makeImage(8, 8, QColor(10, 20, 30, 255));
        eff.setValues({100});
        QColor c = eff.apply(img).pixelColor(4, 4);
        CHECK(c.red() == 245 && c.green() == 235 && c.blue() == 225, "full invert applied via plugin");
        eff.setValues({0});
        QColor c0 = eff.apply(img).pixelColor(4, 4);
        CHECK(c0.red() == 10 && c0.green() == 20 && c0.blue() == 30, "amount 0 leaves pixels unchanged");
    }

    SECTION("Plugin manager loads an external .so if present (non-fatal if absent)");
    {
        const QString dir = QCoreApplication::applicationDirPath() + "/plugins";
        const QString so = dir + "/sample_sepia_plugin.so";
        if (QFile::exists(so)) {
            PluginManager mgr;
            mgr.loadFrom({dir});
            CHECK(!mgr.empty(), "sample plugin discovered and loaded");
            if (!mgr.empty()) {
                auto eff = mgr.effects().front();
                QImage img = makeImage(6, 6, QColor(100, 150, 200, 255));
                QImage out = eff->apply(img).convertToFormat(QImage::Format_ARGB32_Premultiplied);
                CHECK(out.size() == img.size(), "plugin effect returns a same-size image");
                CHECK(imagesDiffer(img, out), "sepia plugin actually changed the pixels");
            }
        } else {
            printf("  (skipped: build/plugins/sample_sepia_plugin.so not present)\n");
        }
    }

    // ---------- TEXT TOOL SWALLOWS ITS OWN KEYS ----------
    SECTION("Text tool captures shortcut letters");
    {
        // ~60% of the alphabet are also tool shortcuts (b, e, s, t, z…). Qt matches
        // those before keyPressEvent, so typing dropped them. The canvas claims
        // ShortcutOverride while the tool wants key input, so the letters arrive.
        Document doc(200, 100);
        doc.activeLayer()->clear(Qt::white);
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        TextTool text;
        canvas.setCurrentTool(&text);

        CHECK(!text.wantsKeyInput(), "text tool doesn't grab keys until you click");
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(10, 40), QPointF(10, 40),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        text.mousePressEvent(QPointF(10, 40), &press, canvas);
        CHECK(text.wantsKeyInput(), "text tool grabs keys once editing");

        // Every letter, including the shortcut ones, must be claimed as text.
        int claimed = 0;
        for (char c = 'a'; c <= 'z'; ++c) {
            QKeyEvent so(QEvent::ShortcutOverride, Qt::Key_A + (c - 'a'), Qt::NoModifier, QString(c));
            QApplication::sendEvent(&canvas, &so);
            if (so.isAccepted()) ++claimed;
        }
        CHECK(claimed == 26, "the canvas claims every letter while typing");

        // But a shortcut chord (Ctrl+…) must still pass through to the app.
        QKeyEvent ctrlA(QEvent::ShortcutOverride, Qt::Key_A, Qt::ControlModifier, QString());
        QApplication::sendEvent(&canvas, &ctrlA);
        CHECK(!ctrlA.isAccepted(), "Ctrl-chords still reach the app while typing");

        // Once editing ends, letters go back to being shortcuts.
        QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        text.keyPressEvent(&esc, canvas);
        CHECK(!text.wantsKeyInput(), "text tool releases keys after Escape");
        QKeyEvent so(QEvent::ShortcutOverride, Qt::Key_B, Qt::NoModifier, QStringLiteral("b"));
        QApplication::sendEvent(&canvas, &so);
        CHECK(!so.isAccepted(), "letters are shortcuts again once editing stops");
    }

    // ---------- COLORS PANEL: More >> / << Less COLLAPSES BACK ----------
    SECTION("Colors panel More/Less");
    {
        // "<< Less" hid the sliders but left the panel at its expanded height,
        // and it could not be shrunk back by hand (issue #9). Check both the
        // docked and the floating case, since they resize through different APIs.
        auto roundTrip = [](bool floating) {
            QMainWindow mw;
            Document doc(64, 64);
            auto *panel = new ColorsPanel;
            panel->setDocument(&doc);
            auto *dock = new QDockWidget("Colors", &mw);
            dock->setWidget(panel);
            mw.addDockWidget(Qt::LeftDockWidgetArea, dock);
            if (floating) { dock->setFloating(true); dock->resize(230, 343); }
            mw.resize(900, 700);
            mw.show();
            QCoreApplication::processEvents();

            QPushButton *more = nullptr;
            for (auto *b : panel->findChildren<QPushButton *>())
                if (b->text().contains("Plus") || b->text().contains("More")) { more = b; break; }
            if (!more) return std::make_tuple(0, 0, 0);

            auto settle = [&] {
                QCoreApplication::processEvents();
                QCoreApplication::sendPostedEvents();
                QCoreApplication::processEvents();
                return panel->sizeHint().height();
            };
            const int before = settle();
            more->click();
            const int expanded = settle();
            more->click();
            const int after = settle();
            return std::make_tuple(before, expanded, after);
        };

        for (bool floating : {false, true}) {
            const auto [before, expanded, after] = roundTrip(floating);
            const char *what = floating ? "floating" : "docked";
            CHECK(expanded > before + 50,
                  floating ? "More >> expands the floating panel" : "More >> expands the docked panel");
            CHECK(after <= before + 5,
                  floating ? "<< Less collapses the floating panel back"
                           : "<< Less collapses the docked panel back");
            printf("    (%s: %d -> %d -> %d)\n", what, before, expanded, after);
        }
    }

    // ---------- COLORS PANEL: PALETTE SERIALIZATION ROUND-TRIPS ----------
    SECTION("Palette serialization");
    {
        // Save-then-load must reproduce the palette exactly, including alpha.
        QVector<QColor> pal = {
            QColor(0x00,0x00,0x00), QColor(0xFF,0x00,0x00),
            QColor(0x12,0x34,0x56), QColor::fromRgba(qRgba(0x80, 0x11, 0x22, 0x33)),
        };
        const QString text = ColorsPanel::paletteToText(pal);
        const QVector<QColor> back = ColorsPanel::paletteFromText(text);
        CHECK(back.size() == pal.size(), "round-trip keeps the colour count");
        bool same = back.size() == pal.size();
        for (int i = 0; i < back.size() && same; ++i)
            same = back[i].rgba() == pal[i].rgba();
        CHECK(same, "round-trip preserves every ARGB value including alpha");

        // 8-digit uppercase AARRGGBB, one per line.
        CHECK(text.startsWith("FF000000\n"), "opaque black serializes as FF000000");

        // Comments (';'), blank lines and malformed lines are skipped.
        const QString messy = "; a comment\n\nFF00FF00\nnothex!!\nFF\n#FF0000FF\n";
        const QVector<QColor> parsed = ColorsPanel::paletteFromText(messy);
        CHECK(parsed.size() == 2, "comments/blank/malformed lines are skipped");
        CHECK(parsed.size() == 2 && parsed[0].rgba() == qRgba(0, 255, 0, 255),
              "green parses from AARRGGBB");
        CHECK(parsed.size() == 2 && parsed[1].rgba() == qRgba(0, 0, 255, 255),
              "a leading '#' is tolerated");

        CHECK(ColorsPanel::defaultPalette().size() == 32, "default palette has 32 colours");
    }

    // ---------- DOCK PANELS NEVER GET WRAPPED IN A GROUP WINDOW ----------
    SECTION("Dock options");
    {
        // QMainWindow::GroupedDragging makes Qt wrap dragged panels in a private
        // QDockWidgetGroupWindow. That wrapper inherits the main window's title,
        // so it shows up as a blank "Untitled - paint.software x.y.z" box, and
        // destroying one while the layout still points at it crashes the app
        // (issue #10). It also lies to isFloating(), which is what broke the
        // Colors panel's collapse when undocked (issue #9). It must stay off.
        QMainWindow mw;
        mw.setDockOptions(QMainWindow::AnimatedDocks
                          | QMainWindow::AllowNestedDocks
                          | QMainWindow::AllowTabbedDocks);
        auto *dock = new QDockWidget("Colors", &mw);
        mw.addDockWidget(Qt::RightDockWidgetArea, dock);
        dock->setFloating(true);
        QCoreApplication::processEvents();

        CHECK(!(mw.dockOptions() & QMainWindow::GroupedDragging),
              "GroupedDragging is off (no blank group windows)");
        CHECK(dock->isFloating(), "an undocked panel reports isFloating() truthfully");
        CHECK(qstrcmp(dock->window()->metaObject()->className(), "QDockWidgetGroupWindow") != 0,
              "an undocked panel is its own window, not a group wrapper");
    }

    // ---------- COLOUR SLIDER LABELS ARE TRANSLATED ----------
    SECTION("Colour slider labels");
    {
        // The letters are French initials in the source: V = Vert (green) and
        // T = Teinte (hue). Left untranslated they read as wrong channels in
        // English, which is what issue #8 was still reporting after the RGB/HSV
        // headers themselves had been fixed.
        auto labelsFor = [](I18n::Lang lang) {
            I18n::setLanguage(lang);
            Document doc(64, 64);
            ColorsPanel panel;
            panel.setDocument(&doc);
            QStringList out;
            for (auto *l : panel.findChildren<QLabel *>()) {
                const QString t = l->text();
                if (t.size() == 2 && t.endsWith(':')) out << t;
            }
            return out;
        };

        const QStringList en = labelsFor(I18n::Lang::English);
        CHECK(en.contains("G:"), "English shows G: for green (not V: for Vert)");
        CHECK(en.contains("H:"), "English shows H: for hue (not T: for Teinte)");
        CHECK(!en.contains("T:"), "no French T: left in the English sliders");

        const QStringList fr = labelsFor(I18n::Lang::French);
        CHECK(fr.contains("V:") && fr.contains("T:"), "French keeps V: (Vert) and T: (Teinte)");
        I18n::setLanguage(I18n::Lang::English);
        printf("    (en: %s | fr: %s)\n", qPrintable(en.join(' ')), qPrintable(fr.join(' ')));
    }

    // ---------- MENU BAR ACCELERATORS ARE UNIQUE IN BOTH LANGUAGES ----------
    SECTION("Menu accelerators");
    {
        // Alt+letter accelerators are case-insensitive, so two menus claiming the
        // same letter do not open — Qt just cycles between them. French used to
        // have "&Fichier" and "E&ffets" both on Alt+F. Translating a menu title
        // moves its accelerator, so this has to hold in every language.
        // Keep this list in step with createMenus().
        const QStringList sourceTitles = {
            "&Fichier", "É&dition", "&Affichage", "&Image",
            "&Calques", "A&justements", "&Effets", "&Options",
        };

        auto collisionFor = [&sourceTitles](I18n::Lang lang) {
            I18n::setLanguage(lang);
            QHash<QChar, QString> taken;
            QString clash;
            for (const QString &src : sourceTitles) {
                const QString title = I18n::t(src);
                const int amp = title.indexOf('&');
                if (amp < 0 || amp + 1 >= title.size()) continue;
                const QChar key = title.at(amp + 1).toLower();
                if (taken.contains(key))
                    clash += QStringLiteral("%1 vs %2 on Alt+%3; ")
                                 .arg(taken.value(key), title, QString(key.toUpper()));
                else
                    taken.insert(key, title);
            }
            return clash;
        };

        const QString frClash = collisionFor(I18n::Lang::French);
        const QString enClash = collisionFor(I18n::Lang::English);
        CHECK(frClash.isEmpty(), "no duplicate menu accelerators in French");
        CHECK(enClash.isEmpty(), "no duplicate menu accelerators in English");
        if (!frClash.isEmpty()) printf("    (fr: %s)\n", qPrintable(frClash));
        if (!enClash.isEmpty()) printf("    (en: %s)\n", qPrintable(enClash));
        I18n::setLanguage(I18n::Lang::English);
    }

    // ---------- PIXEL COORDINATE CONVERSION ----------
    SECTION("Pixel coordinate conversion");
    {
        // toPixelPos floors: the pixel under x=5.7 is pixel 5 (it spans [5,6)),
        // not 6 as QPointF::toPoint() (round-to-nearest) would give. This was the
        // Pencil off-by-one in pixel art (PR #5).
        CHECK(toPixelPos(QPointF(5.7, 5.7)) == QPoint(5, 5), "toPixelPos floors 5.7 to 5");
        CHECK(toPixelPos(QPointF(5.2, 5.9)) == QPoint(5, 5), "toPixelPos floors within a pixel");
        CHECK(toPixelPos(QPointF(0.0, 0.0)) == QPoint(0, 0), "toPixelPos of an exact edge");
        CHECK(toPixelPos(QPointF(-0.3, -0.3)) == QPoint(-1, -1), "toPixelPos floors negatives");

        // The Pencil now marks the pixel actually under the cursor.
        Document doc(10, 10);
        doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::black);
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        PencilTool pencil;
        // A single click at (3.7, 4.2): pixel (3, 4) must be the one painted.
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(3.7, 4.2), QPointF(3.7, 4.2),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        pencil.mousePressEvent(QPointF(3.7, 4.2), &press, canvas);
        QMouseEvent rel(QEvent::MouseButtonRelease, QPointF(3.7, 4.2), QPointF(3.7, 4.2),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        pencil.mouseReleaseEvent(QPointF(3.7, 4.2), &rel, canvas);
        CHECK(doc.activeLayer()->image().pixelColor(3, 4) == QColor(Qt::black),
              "Pencil paints the pixel under the cursor, not the rounded one");
        CHECK(doc.activeLayer()->image().pixelColor(4, 4) != QColor(Qt::black),
              "Pencil does not paint the neighbouring (rounded) pixel");

        // A movement delta must stay round-to-nearest, not floor: a sub-pixel
        // nudge shouldn't jump a whole pixel (that would drift the Move tools).
        CHECK((QPointF(-0.001, -0.001)).toPoint() == QPoint(0, 0),
              "a tiny negative drag delta stays at zero (round, not floor)");
    }

    // ---------- NEW LAYERS ARE TRANSPARENT ----------
    SECTION("New layers are transparent");
    {
        Document doc(20, 20);
        // The initial Background layer is white/opaque — that must not change.
        CHECK(doc.layerAt(0)->image().pixelColor(5, 5) == QColor(Qt::white),
              "the Background layer starts white");
        // Draw on it, add a layer: the new layer must be transparent so the
        // artwork below shows through (issue #3 — added layers were opaque white).
        { QPainter p(&doc.layerAt(0)->image()); p.fillRect(0, 0, 20, 20, Qt::red); }
        const int idx = doc.addLayer("Layer 2");
        CHECK(doc.layerAt(idx)->image().pixelColor(10, 10).alpha() == 0,
              "an added layer is fully transparent");
        const QColor flat = doc.flatten().convertToFormat(QImage::Format_ARGB32).pixelColor(10, 10);
        CHECK(flat.red() > 200 && flat.green() < 60,
              "artwork on a lower layer shows through a new layer");
    }

    SECTION("Pasted image becomes a canvas-sized layer, movable without clipping");
    {
        // A pasted image smaller than the canvas must land on a canvas-sized
        // layer, so it can be moved across the canvas instead of being clipped to
        // its own little box (issue #18).
        Document doc(200, 100);
        QImage paste(40, 40, QImage::Format_ARGB32);
        paste.fill(Qt::red);
        const int idx = doc.addLayer(paste, "Calque collé");
        CHECK(doc.layerAt(idx)->image().size() == QSize(200, 100),
              "pasted layer is canvas-sized, not paste-sized");
        CHECK(doc.layerAt(idx)->image().pixelColor(10, 10) == QColor(Qt::red),
              "the pasted pixels are placed at the top-left");
        CHECK(doc.layerAt(idx)->image().pixelColor(120, 50).alpha() == 0,
              "the rest of the canvas-sized layer is transparent");

        // Drive the REAL Move tool end-to-end (no selection = whole-layer move),
        // which is the code path that used to clip to the paste box. Grab inside
        // the red block and drag it +100px right; it must arrive intact.
        {
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            canvas.setZoom(1.0);
            MoveTool tool;
            const QPointF from(20, 20), to(120, 20);
            QMouseEvent e1(QEvent::MouseButtonPress, from, from, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            tool.mousePressEvent(from, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);

            const QImage &after = doc.layerAt(idx)->image();
            CHECK(after.pixelColor(130, 20) == QColor(Qt::red),
                  "Move tool: pasted block travels +100px intact (right edge not clipped)");
            CHECK(after.pixelColor(10, 20).alpha() == 0,
                  "Move tool: the block really moved (old position now empty)");
        }
    }

    SECTION("Clipboard paste preserves oversized content and is one undoable operation");
    {
        Document doc(200, 100);
        QImage image(240, 120, QImage::Format_ARGB32);
        image.fill(QColor(20, 80, 160, 128));
        image.setPixelColor(239, 119, Qt::red);
        doc.selection().selectRect(QRect(5, 7, 10, 11));
        const QImage mask = doc.selection().mask();
        const QImage original = doc.activeLayer()->image();
        doc.pasteImage(image, "paste", false);
        CHECK(doc.size() == QSize(200, 100), "Keep does not resize the canvas");
        CHECK(doc.activeLayer()->image().size() == image.size(), "Keep retains all pasted pixels");
        CHECK(doc.activeLayer()->image().format() == QImage::Format_ARGB32_Premultiplied,
              "clipboard paste stores premultiplied pixels");
        CHECK(doc.activeLayer()->image().pixelColor(239,119) == QColor(Qt::red),
              "the off-canvas corner survives paste");
        CHECK(!doc.selection().hasSelection(), "accepted paste clears a stale selection");
        CHECK(doc.history().currentIndex() == 1, "paste is a single history action");
        doc.history().undo();
        CHECK(doc.layerCount() == 1 && doc.activeLayerIndex() == 0, "undo restores stack and active layer");
        CHECK(doc.selection().mask() == mask, "undo restores the pre-paste selection");
        doc.history().redo();
        CHECK(doc.activeLayer()->image().pixelColor(239,119) == QColor(Qt::red), "redo retains hidden pixels");
        doc.activeLayer()->setOffset(QPoint(-40,-20));
        QImage next(260, 80, QImage::Format_ARGB32_Premultiplied);
        next.fill(Qt::transparent);
        doc.pasteImage(next, "expanded", true);
        CHECK(doc.size() == QSize(260, 100), "Expand grows only dimensions that need it");
        CHECK(doc.layerAt(0)->image().copy(original.rect()) == original, "Expand does not resample old artwork");
        CHECK(doc.layerAt(1)->offset() == QPoint(-40,-20), "Expand preserves existing layer positions");
        CHECK(doc.layerAt(1)->image().pixelColor(239,119) == QColor(Qt::red), "Expand retains pre-existing hidden content");
        doc.history().undo();
        CHECK(doc.size() == QSize(200,100) && doc.layerCount() == 2 && doc.activeLayerIndex() == 1,
              "one undo reverses expansion and paste together");
    }

    // ---------- LAYER MOVE / FLIP / IMPORT ----------
    SECTION("Layer move / flip / import");
    {
        {   // move reorders the stack
            Document doc(20, 20);
            doc.layerAt(0)->setName("A");
            doc.addLayer();
            doc.layerAt(1)->setName("B");
            doc.moveLayer(0, 1);
            CHECK(doc.layerAt(0)->name() == "B" && doc.layerAt(1)->name() == "A",
                  "moving a layer up reorders the stack");
        }
        {   // per-layer horizontal flip swaps left/right, others untouched
            Document doc(4, 1);
            doc.addLayer();   // a second layer that must NOT move
            doc.layerAt(1)->image().fill(Qt::green);
            const QImage other = doc.layerAt(1)->image().copy();
            QImage &img = doc.layerAt(0)->image();
            img.fill(Qt::transparent);
            img.setPixelColor(0, 0, Qt::red);
            img.setPixelColor(3, 0, Qt::blue);
            doc.layerAt(0)->setImage(img.mirrored(true, false));
            CHECK(doc.layerAt(0)->image().pixelColor(0, 0).blue() > 200
                  && doc.layerAt(0)->image().pixelColor(3, 0).red() > 200,
                  "per-layer flip mirrors that layer");
            CHECK(doc.layerAt(1)->image() == other, "per-layer flip leaves other layers alone");
        }
        {   // import adds an image as a new named layer
            Document doc(10, 10);
            const int before = doc.layerCount();
            QImage img(6, 6, QImage::Format_ARGB32);
            img.fill(Qt::green);
            const int idx = doc.addLayer(img, "imported.png");
            CHECK(doc.layerCount() == before + 1 && doc.layerAt(idx)->name() == "imported.png",
                  "importing a file adds it as a new layer");
        }
    }

    // ---------- SELECTION MODES + GROW/SHRINK/FEATHER ----------
    SECTION("Selection modes and modify");
    {
        auto area = [](const Selection &s, int w, int h) {
            long n = 0;
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                    if (s.isSelected(x, y)) ++n;
            return n;
        };
        // The shared modifier→mode helper every selection tool uses — the
        // detailed Paint.NET mapping is asserted in the selection-modes section.
        CHECK(selectionModeFor(Qt::NoModifier) == SelectionMode::Replace, "no modifier = replace");
        CHECK(selectionModeFor(Qt::ControlModifier) == SelectionMode::Add, "ctrl = add");
        CHECK(selectionModeFor(Qt::AltModifier) == SelectionMode::Subtract, "alt = subtract");

        {   // intersect keeps only the overlap
            Selection s(100, 100);
            s.selectRect(QRect(10, 10, 40, 40));
            s.selectRect(QRect(30, 30, 40, 40), SelectionMode::Intersect);
            CHECK(area(s, 100, 100) == 400, "intersect keeps the 20x20 overlap");
        }
        {   // grow enlarges, shrink reduces
            Selection s(100, 100);
            s.selectRect(QRect(40, 40, 20, 20));
            const long base = area(s, 100, 100);
            s.expand(5);
            CHECK(area(s, 100, 100) > base, "grow enlarges the selection");
            Selection s2(100, 100);
            s2.selectRect(QRect(40, 40, 20, 20));
            s2.contract(3);
            CHECK(area(s2, 100, 100) < base, "shrink reduces the selection");
        }
    }

    // ---------- FILL SELECTION ----------
    SECTION("Fill selection");
    {
        auto fillMasked = [](Document &doc) {
            // Mirror MainWindow::fillSelection's masked path.
            Layer *layer = doc.activeLayer();
            QImage fill(layer->image().size(), QImage::Format_ARGB32_Premultiplied);
            fill.fill(doc.primaryColor());
            fill = doc.selection().getMaskedImage(fill, layer->offset());
            QPainter p(&layer->image());
            p.drawImage(0, 0, fill);
        };
        auto redCount = [](const QImage &in) {
            const QImage img = in.convertToFormat(QImage::Format_ARGB32);
            long n = 0;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    const QColor c = img.pixelColor(x, y);
                    if (c.red() > 200 && c.green() < 60 && c.blue() < 60) ++n;
                }
            return n;
        };

        Document doc(40, 40);
        doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::red);
        doc.selection().selectRect(QRect(10, 10, 20, 20));   // 400 px
        fillMasked(doc);
        CHECK(redCount(doc.activeLayer()->image()) == 400, "fill selection paints only the 400 selected pixels");

        // No selection -> whole layer is fair game (MainWindow fills the rect).
        Document doc2(30, 30);
        doc2.activeLayer()->clear(Qt::white);
        doc2.setPrimaryColor(Qt::red);
        { QPainter p(&doc2.activeLayer()->image()); p.fillRect(doc2.activeLayer()->image().rect(), doc2.primaryColor()); }
        CHECK(redCount(doc2.activeLayer()->image()) == 900, "fill with no selection covers the whole layer");
    }

    // ---------- PASTE TEXT INTO THE TEXT TOOL ----------
    SECTION("Text tool: paste");
    {
        auto inkWidth = [](const QImage &img) {
            int x0 = INT_MAX, x1 = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x)
                    if (qGray(img.pixel(x, y)) < 128) { x0 = std::min(x0, x); x1 = std::max(x1, x); }
            return x1 < 0 ? 0 : x1 - x0 + 1;
        };
        auto inkHeight = [](const QImage &img) {
            int y0 = INT_MAX, y1 = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x)
                    if (qGray(img.pixel(x, y)) < 128) { y0 = std::min(y0, y); y1 = std::max(y1, y); }
            return y1 < 0 ? 0 : y1 - y0 + 1;
        };

        {   // pasting while editing appends at the caret and renders
            Document doc(400, 120);
            doc.activeLayer()->clear(Qt::white);
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            TextTool tool;
            canvas.setCurrentTool(&tool);
            QMouseEvent pr(QEvent::MouseButtonPress, QPointF(10, 60), QPointF(10, 60),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            tool.mousePressEvent(QPointF(10, 60), &pr, canvas);
            tool.insertText("pasted", canvas);
            tool.commitText(canvas);
            CHECK(inkWidth(doc.activeLayer()->image()) > 20, "pasted text is drawn while editing");
        }
        {   // paste outside editing is a no-op — no text conjured from nowhere
            Document doc(400, 120);
            doc.activeLayer()->clear(Qt::white);
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            TextTool tool;
            canvas.setCurrentTool(&tool);
            tool.insertText("ghost", canvas);
            tool.commitText(canvas);
            CHECK(inkWidth(doc.activeLayer()->image()) == 0, "paste does nothing when not editing");
        }
        {   // pasted CRLF becomes a real line break
            Document doc(400, 200);
            doc.activeLayer()->clear(Qt::white);
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            TextTool tool;
            canvas.setCurrentTool(&tool);
            QMouseEvent pr(QEvent::MouseButtonPress, QPointF(10, 40), QPointF(10, 40),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            tool.mousePressEvent(QPointF(10, 40), &pr, canvas);
            tool.insertText("a\r\nb", canvas);
            tool.commitText(canvas);
            CHECK(inkHeight(doc.activeLayer()->image()) > 30, "pasted CRLF makes two lines");
        }
    }

    // ---------- MOVE TOOL: RESIZING THE SELECTED PIXELS ----------
    SECTION("Move tool: resize selected pixels");
    {
        // Dragging a handle must stretch or squash the artwork itself, like
        // paint.net's Move Selected Pixels. The tool had no handles at all, so
        // shrinking a selected area was impossible with any tool.
        auto redBounds = [](const QImage &img) {
            int x0 = INT_MAX, y0 = INT_MAX, x1 = -1, y1 = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    const QColor c = img.pixelColor(x, y);
                    if (c.red() > 150 && c.green() < 90 && c.blue() < 90 && c.alpha() > 100) {
                        x0 = std::min(x0, x); y0 = std::min(y0, y);
                        x1 = std::max(x1, x); y1 = std::max(y1, y);
                    }
                }
            return x1 < 0 ? QRect() : QRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
        };
        auto dragRed = [&](QPointF grab, QPointF to) {
            Document doc(200, 200);
            doc.activeLayer()->clear(Qt::white);
            { QPainter p(&doc.activeLayer()->image()); p.fillRect(QRect(40, 40, 60, 60), Qt::red); }
            doc.selection().selectRect(QRect(40, 40, 60, 60));
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            canvas.setZoom(1.0);
            MoveTool tool;
            QMouseEvent e1(QEvent::MouseButtonPress, grab, grab, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            tool.mousePressEvent(grab, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);
            return redBounds(doc.activeLayer()->image());
        };
        auto near = [](int a, int b) { return std::abs(a - b) <= 3; };

        QRect r = dragRed(QPointF(99, 99), QPointF(139, 139));
        CHECK(near(r.width(), 100) && near(r.height(), 100), "corner handle grows the artwork");
        r = dragRed(QPointF(99, 99), QPointF(69, 69));
        CHECK(near(r.width(), 30) && near(r.height(), 30), "corner handle shrinks the artwork");
        r = dragRed(QPointF(70, 99), QPointF(70, 60));
        CHECK(near(r.width(), 60) && r.height() < 30, "bottom handle squashes the artwork flat");
        r = dragRed(QPointF(99, 70), QPointF(159, 70));
        CHECK(near(r.width(), 120) && near(r.height(), 60), "right handle stretches width only");
        // Pressing away from any handle must still move, not resize.
        r = dragRed(QPointF(70, 70), QPointF(100, 100));
        CHECK(near(r.width(), 60) && near(r.height(), 60) && near(r.x(), 70) && near(r.y(), 70),
              "pressing inside still moves the artwork unscaled");
    }

    // ---------- UNDO/REDO OF A PIXEL RESIZE ----------
    SECTION("Move tool: undo/redo a resize");
    {
        Document doc(200, 200);
        doc.activeLayer()->clear(Qt::white);
        { QPainter p(&doc.activeLayer()->image()); p.fillRect(QRect(40, 40, 60, 60), Qt::red); }
        doc.selection().selectRect(QRect(40, 40, 60, 60));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.setZoom(1.0);

        auto redRect = [&]() {
            const QImage &img = doc.activeLayer()->image();
            int x0 = INT_MAX, y0 = INT_MAX, x1 = -1, y1 = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    const QColor c = img.pixelColor(x, y);
                    if (c.red() > 150 && c.green() < 90 && c.blue() < 90 && c.alpha() > 100) {
                        x0 = std::min(x0, x); y0 = std::min(y0, y);
                        x1 = std::max(x1, x); y1 = std::max(y1, y);
                    }
                }
            return x1 < 0 ? QRect() : QRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
        };

        MoveTool tool;
        const QPointF grab(99, 99), to(69, 69);
        QMouseEvent e1(QEvent::MouseButtonPress, grab, grab, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        tool.mousePressEvent(grab, &e1, canvas);
        QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        tool.mouseMoveEvent(to, &e2, canvas);
        QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        tool.mouseReleaseEvent(to, &e3, canvas);

        CHECK(redRect().width() < 40, "resize shrank the artwork");
        // One drag must leave exactly one undo step, and it must put the pixels back.
        CHECK(doc.history().canUndo(), "a resize is undoable");
        doc.history().undo();
        CHECK(redRect() == QRect(40, 40, 60, 60), "undo restores the original artwork");
        CHECK(doc.history().canRedo(), "a resize is redoable");
        doc.history().redo();
        CHECK(redRect().width() < 40, "redo re-applies the resize");
    }

    // ---------- MOVE TOOL TARGETS THE ACTIVE LAYER ----------
    SECTION("Move tool acts on the selected layer");
    {
        // The selection is document-wide, so it can be drawn while one layer is
        // active and then used after switching to another. Whatever is on screen,
        // the edit must land on the SELECTED layer and leave the others alone.
        auto colourBounds = [](const QImage &img, QColor want) {
            int x0 = INT_MAX, y0 = INT_MAX, x1 = -1, y1 = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    const QColor c = img.pixelColor(x, y);
                    if (c.alpha() > 100 && std::abs(c.red() - want.red()) < 60
                        && std::abs(c.green() - want.green()) < 60
                        && std::abs(c.blue() - want.blue()) < 60) {
                        x0 = std::min(x0, x); y0 = std::min(y0, y);
                        x1 = std::max(x1, x); y1 = std::max(y1, y);
                    }
                }
            return x1 < 0 ? QRect() : QRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
        };
        // Two layers, same square: red underneath, blue on top.
        auto build = [](Document &doc) {
            doc.activeLayer()->clear(Qt::transparent);
            { QPainter p(&doc.layerAt(0)->image()); p.fillRect(QRect(40, 40, 60, 60), Qt::red); }
            doc.addLayer();
            { QPainter p(&doc.layerAt(1)->image()); p.fillRect(QRect(40, 40, 60, 60), Qt::blue); }
            doc.setActiveLayer(0);
            doc.selection().selectRect(QRect(40, 40, 60, 60));   // drawn on layer 0
            doc.setActiveLayer(1);                               // but layer 1 is selected
        };
        auto drag = [](Document &doc, QPointF from, QPointF to) {
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            canvas.setZoom(1.0);
            MoveTool tool;
            QMouseEvent e1(QEvent::MouseButtonPress, from, from, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            tool.mousePressEvent(from, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);
        };

        {   // resizing via a handle
            Document doc(200, 200);
            build(doc);
            drag(doc, QPointF(99, 99), QPointF(69, 69));
            const QRect blue = colourBounds(doc.layerAt(1)->image(), Qt::blue);
            const QRect red = colourBounds(doc.layerAt(0)->image(), Qt::red);
            CHECK(blue.width() < 40 && blue.height() < 40, "resize scales the SELECTED layer");
            CHECK(red == QRect(40, 40, 60, 60), "resize leaves the other layer untouched");
        }
        {   // plain move
            Document doc(200, 200);
            build(doc);
            drag(doc, QPointF(70, 70), QPointF(100, 100));
            const QRect blue = colourBounds(doc.layerAt(1)->image(), Qt::blue);
            const QRect red = colourBounds(doc.layerAt(0)->image(), Qt::red);
            CHECK(blue.x() > 55, "move shifts the SELECTED layer");
            CHECK(red == QRect(40, 40, 60, 60), "move leaves the other layer untouched");
        }
        {   // a locked layer must refuse both
            Document doc(200, 200);
            build(doc);
            doc.layerAt(1)->setLocked(true);
            const QImage before = doc.layerAt(1)->image().copy();
            drag(doc, QPointF(99, 99), QPointF(69, 69));
            CHECK(doc.layerAt(1)->image() == before, "a locked layer is not resized");
        }
    }

    SECTION("Brush keeps drawing when alternating primary/secondary buttons (issue #17)");
    {
        // Releasing one of two held buttons must not cut the stroke, and a second
        // button pressed mid-stroke must not restart it. Paul-Coy saw the mouse
        // stop drawing at the primary<->secondary transition.
        Document doc(64, 64);
        doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::black);
        doc.setSecondaryColor(Qt::red);
        CanvasWidget canvas; canvas.setDocument(&doc); canvas.setZoom(1.0);
        BrushTool brush; brush.setBrushSize(6);

        const QPointF a(10, 32), b(30, 32), c(50, 32);
        // Press LEFT and draw a->b.
        QMouseEvent p1(QEvent::MouseButtonPress, a, a, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        brush.mousePressEvent(a, &p1, canvas);
        QMouseEvent m1(QEvent::MouseMove, b, b, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        brush.mouseMoveEvent(b, &m1, canvas);
        // Press RIGHT too (both held) — must NOT restart the stroke.
        QMouseEvent p2(QEvent::MouseButtonPress, b, b, Qt::RightButton, Qt::LeftButton | Qt::RightButton, Qt::NoModifier);
        brush.mousePressEvent(b, &p2, canvas);
        // Release LEFT while RIGHT is still held — the stroke must continue.
        QMouseEvent r1(QEvent::MouseButtonRelease, b, b, Qt::LeftButton, Qt::RightButton, Qt::NoModifier);
        brush.mouseReleaseEvent(b, &r1, canvas);
        // Move to c — this segment must still paint (drawing didn't stop).
        QMouseEvent m2(QEvent::MouseMove, c, c, Qt::NoButton, Qt::RightButton, Qt::NoModifier);
        brush.mouseMoveEvent(c, &m2, canvas);
        // Release the LAST button — now the stroke ends.
        QMouseEvent r2(QEvent::MouseButtonRelease, c, c, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
        brush.mouseReleaseEvent(c, &r2, canvas);

        const QImage &img = doc.activeLayer()->image();
        // First half = primary (black); the segment must actually be painted.
        CHECK(qGray(img.pixel(20, 32)) < 120, "first half painted in the primary colour");
        // Second half painted (no gap) AND in the SECONDARY colour — the button
        // switch now changes colour mid-line instead of stopping (issue #17).
        const QColor c2 = img.pixelColor(40, 32);
        CHECK(c2.alpha() > 0 && c2.red() > 180 && c2.green() < 90 && c2.blue() < 90,
              "stroke continued past the switch and turned secondary (no gap)");
    }

    // ---------- MOVE SELECTION TOOL ----------
    SECTION("Move Selection tool");
    {
        auto dragSelection = [](QRect sel, QPointF from, QPoint delta, double zoom) {
            Document doc(200, 200);
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            canvas.setZoom(zoom);
            doc.selection().selectRect(sel);
            MoveSelectionTool tool;
            const QPointF to = from + QPointF(delta.x(), delta.y());
            QMouseEvent e1(QEvent::MouseButtonPress, from, from, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            tool.mousePressEvent(from, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);
            return doc.selection().boundingRect();
        };

        // The resize handles used to span a flat 6 canvas pixels, so on a small
        // selection the corner zones met in the middle: every drag resized the
        // marquee and the selection could not be moved at all.
        const QRect small(10, 10, 20, 20);
        CHECK(dragSelection(small, QPointF(15, 15), QPoint(20, 10), 1.0) == small.translated(20, 10),
              "small selection moves when dragged near a corner");
        CHECK(dragSelection(small, QPointF(20, 20), QPoint(20, 10), 1.0) == small.translated(20, 10),
              "small selection moves when dragged from its centre");
        const QRect big(20, 20, 80, 80);
        CHECK(dragSelection(big, QPointF(60, 60), QPoint(10, 10), 1.0) == big.translated(10, 10),
              "large selection moves when dragged from its centre");
        // The grab zone is screen-space, so it must hold at any zoom.
        CHECK(dragSelection(small, QPointF(14, 14), QPoint(20, 10), 4.0) == small.translated(20, 10),
              "selection moves when zoomed in");
        CHECK(dragSelection(small, QPointF(20, 20), QPoint(20, 10), 0.25) == small.translated(20, 10),
              "selection moves when zoomed out");
        // A press outside the marquee must do nothing.
        CHECK(dragSelection(big, QPointF(150, 150), QPoint(10, 10), 1.0) == big,
              "pressing outside the selection leaves it alone");
        // Grabbing a corner exactly must still resize rather than move.
        {
            const QRect r = dragSelection(big, QPointF(99, 99), QPoint(40, 40), 1.0);
            CHECK(r.width() > big.width() + 20 && r.height() > big.height() + 20,
                  "grabbing the corner handle still resizes the marquee");
        }
    }

    // ---------- MOVE TOOL: ROTATION + MODIFIER KEYS ----------
    SECTION("Move tool: rotation and modifier keys");
    {
        auto redBounds = [](const QImage &img) {
            int x0 = INT_MAX, y0 = INT_MAX, x1 = -1, y1 = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    const QColor c = img.pixelColor(x, y);
                    if (c.red() > 150 && c.green() < 90 && c.blue() < 90 && c.alpha() > 100) {
                        x0 = std::min(x0, x); y0 = std::min(y0, y);
                        x1 = std::max(x1, x); y1 = std::max(y1, y);
                    }
                }
            return x1 < 0 ? QRect() : QRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
        };
        auto countRed = [](const QImage &img) {
            int n = 0;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x) {
                    const QColor c = img.pixelColor(x, y);
                    if (c.red() > 150 && c.green() < 90 && c.blue() < 90 && c.alpha() > 100) ++n;
                }
            return n;
        };
        auto near = [](int a, int b) { return std::abs(a - b) <= 4; };

        // Ctrl at press moves a COPY: the original square stays where it was and a
        // second one appears at the drop point.
        {
            Document doc(200, 200);
            doc.activeLayer()->clear(Qt::white);
            { QPainter p(&doc.activeLayer()->image()); p.fillRect(QRect(40, 40, 40, 40), Qt::red); }
            doc.selection().selectRect(QRect(40, 40, 40, 40));
            CanvasWidget canvas; canvas.setDocument(&doc); canvas.setZoom(1.0);
            MoveTool tool;
            const QPointF from(60, 60), to(160, 160);   // press inside, drag far away
            QMouseEvent e1(QEvent::MouseButtonPress, from, from, Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
            tool.mousePressEvent(from, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::ControlModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::ControlModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);
            const QImage &img = doc.activeLayer()->image();
            CHECK(img.pixelColor(60, 60).red() > 150, "Ctrl-move leaves the original pixels in place");
            CHECK(img.pixelColor(160, 160).red() > 150, "Ctrl-move drops a copy at the new spot");
        }

        // Shift while resizing a corner keeps the original aspect ratio (square →
        // square) even when the drag is lopsided.
        {
            Document doc(200, 200);
            doc.activeLayer()->clear(Qt::white);
            { QPainter p(&doc.activeLayer()->image()); p.fillRect(QRect(40, 40, 40, 40), Qt::red); }
            doc.selection().selectRect(QRect(40, 40, 40, 40));
            CanvasWidget canvas; canvas.setDocument(&doc); canvas.setZoom(1.0);
            MoveTool tool;
            const QPointF grab(79, 79), to(139, 110);   // bottom-right corner, uneven drag
            QMouseEvent e1(QEvent::MouseButtonPress, grab, grab, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
            tool.mousePressEvent(grab, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);
            const QRect r = redBounds(doc.activeLayer()->image());
            CHECK(near(r.width(), r.height()), "Shift-resize keeps the aspect ratio square");
        }

        // Alt while resizing grows symmetrically about the centre: the opposite edge
        // moves out too, so the centre of the square stays put.
        {
            Document doc(200, 200);
            doc.activeLayer()->clear(Qt::white);
            { QPainter p(&doc.activeLayer()->image()); p.fillRect(QRect(60, 60, 40, 40), Qt::red); }
            doc.selection().selectRect(QRect(60, 60, 40, 40));
            const QPoint centreBefore = QRect(60, 60, 40, 40).center();
            CanvasWidget canvas; canvas.setDocument(&doc); canvas.setZoom(1.0);
            MoveTool tool;
            const QPointF grab(99, 79), to(120, 79);   // right edge dragged out
            QMouseEvent e1(QEvent::MouseButtonPress, grab, grab, Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
            tool.mousePressEvent(grab, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::LeftButton, Qt::NoButton, Qt::AltModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);
            const QRect r = redBounds(doc.activeLayer()->image());
            CHECK(r.left() < 60, "Alt-resize also moves the opposite (left) edge outward");
            CHECK(near(r.center().x(), centreBefore.x()), "Alt-resize keeps the centre fixed");
        }

        // Right-drag rotates the content about the box centre: a 90°-ish rotation of
        // a wide rectangle makes the bounding box taller than it is wide.
        {
            Document doc(200, 200);
            doc.activeLayer()->clear(Qt::white);
            { QPainter p(&doc.activeLayer()->image()); p.fillRect(QRect(60, 90, 80, 20), Qt::red); }
            doc.selection().selectRect(QRect(60, 90, 80, 20));
            const QRect before = redBounds(doc.activeLayer()->image());
            const int redBefore = countRed(doc.activeLayer()->image());
            CanvasWidget canvas; canvas.setDocument(&doc); canvas.setZoom(1.0);
            MoveTool tool;
            const QPointF c = QRectF(QRect(60, 90, 80, 20)).center();
            const QPointF from = c + QPointF(60, 0);   // to the right of centre
            const QPointF to   = c + QPointF(0, 60);   // straight below → ~90° turn
            QMouseEvent e1(QEvent::MouseButtonPress, from, from, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
            tool.mousePressEvent(from, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, to, to, Qt::NoButton, Qt::RightButton, Qt::NoModifier);
            tool.mouseMoveEvent(to, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, to, to, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
            tool.mouseReleaseEvent(to, &e3, canvas);
            const QRect after = redBounds(doc.activeLayer()->image());
            CHECK(before.width() > before.height(), "sanity: started wide");
            CHECK(after.height() > after.width(), "rotation turns the wide box tall");
            CHECK(std::abs(countRed(doc.activeLayer()->image()) - redBefore) < redBefore / 2,
                  "rotation preserves roughly the same amount of content");
            CHECK(doc.history().canUndo(), "a rotation is undoable");
        }
    }

    // ---------- CANVAS VIEW CENTRING ----------
    SECTION("Canvas view centring");
    {
        Document doc(800, 600);
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.show();   // hidden widgets never get a resize event (CI runs offscreen)
        const int rs = canvas.rulerSize();

        auto resizeTo = [&](int w, int h) { canvas.resize(w, h); QCoreApplication::processEvents(); };
        auto isCentred = [&](int w, int h) {
            const QPointF want((w - rs - doc.width() * canvas.zoom()) / 2.0,
                               (h - rs - doc.height() * canvas.zoom()) / 2.0);
            return std::abs(canvas.pan().x() - want.x()) < 0.51
                && std::abs(canvas.pan().y() - want.y()) < 0.51;
        };

        resizeTo(1200, 800);
        canvas.resetToDefaultView();
        CHECK(isCentred(1200, 800), "resetToDefaultView centres the image");

        // The window manager resizes us *after* startup (maximise), so centring
        // once at construction leaves the image off-centre unless we follow.
        resizeTo(1920, 1040);
        CHECK(isCentred(1920, 1040), "image re-centres when the window is maximised");
        resizeTo(900, 650);
        CHECK(isCentred(900, 650), "image re-centres when the window is restored small");

        // A margin clamp used to shove a snugly-fitting image 40px off-centre.
        resizeTo(800 + rs + 30, 600 + rs + 30);
        CHECK(std::abs(canvas.pan().x() - 15.0) < 0.51 && std::abs(canvas.pan().y() - 15.0) < 0.51,
              "snug window still centres exactly (no margin clamp)");

        // Once the user places the view themselves, a resize must not steal it.
        resizeTo(1200, 800);
        canvas.setPan(QPointF(5, 7));
        resizeTo(1500, 900);
        CHECK(canvas.pan() == QPointF(5, 7), "a user pan survives a resize (no auto-centring)");
        canvas.resetToDefaultView();
        CHECK(isCentred(1500, 900), "resetToDefaultView re-enables centring after a pan");
    }

    SECTION("Zoom reuses the flattened document cache");
    {
        Document doc(32, 32);
        doc.activeLayer()->clear(Qt::red);
        CanvasWidget canvas;
        canvas.resize(80, 80);
        canvas.setShowRulers(false);
        canvas.setDocument(&doc);
        canvas.setPan(QPointF(0, 0));

        QImage target(canvas.size(), QImage::Format_ARGB32_Premultiplied);
        canvas.render(&target);                    // populate the cache
        doc.activeLayer()->clear(Qt::blue);        // mutate without notification
        canvas.setZoom(2.0);
        canvas.render(&target);
        CHECK(target.pixelColor(25, 25).red() > 180,
              "zoom-only changes keep the cached document pixels");

        doc.notifyContentChanged();
        canvas.render(&target);
        CHECK(target.pixelColor(25, 25).blue() > 180,
              "document changes still invalidate the cached document pixels");
    }

    SECTION("Resize dialog keeps typed decimals");
    {
        // Typing "8.5" into a Print Size field used to drop the ".5": the first
        // keystroke fired valueChanged, syncFromPixels() wrote the value back to
        // the same box mid-edit, and the reformat reset the text and cursor
        // (issue #14). The fix is keyboardTracking(false) on the decimal spins,
        // so they only commit on Enter / focus-out. Guard that invariant.
        ResizeDialog dlg(800, 600, 96);
        int checked = 0;
        for (QDoubleSpinBox *sb : dlg.findChildren<QDoubleSpinBox *>()) {
            CHECK(!sb->keyboardTracking(),
                  "a resize-dialog decimal field does not re-format on every keystroke");
            ++checked;
        }
        CHECK(checked >= 3, "found the print-size and percent decimal fields");
    }

    SECTION("Brush size: fractional, up to 2000, editable dropdown");
    {
        // Paint.NET's brush size is an editable dropdown that accepts decimals
        // (it antialiases sub-integer sizes) and ranges up to 2000. Ours used to
        // be an integer spin capped at 500 (#12).
        BrushTool brush;
        brush.setBrushSize(6.5);
        CHECK(std::abs(brush.brushSize() - 6.5) < 1e-9, "brush size keeps a decimal value");
        brush.setBrushSize(2000);
        CHECK(std::abs(brush.brushSize() - 2000.0) < 1e-9, "brush size reaches 2000");
        brush.setBrushSize(5000);
        CHECK(std::abs(brush.brushSize() - 2000.0) < 1e-9, "brush size clamps above 2000");
        brush.setBrushSize(0.2);
        CHECK(std::abs(brush.brushSize() - 1.0) < 1e-9, "brush size clamps below 1");

        // The panel's control: an editable combo with presets, wired so typing a
        // value commits it to the tool.
        ToolOptionsPanel panel;
        panel.setTool(&brush);
        QComboBox *sizeCombo = nullptr;
        for (QComboBox *c : panel.findChildren<QComboBox *>()) {
            if (c->isEditable() && c->findText("2000") >= 0) { sizeCombo = c; break; }
        }
        CHECK(sizeCombo != nullptr, "brush size is an editable dropdown listing 2000");
        if (sizeCombo) {
            CHECK(sizeCombo->count() >= 50, "the dropdown offers a full range of presets");
            // Simulate the user typing "12.5" and committing with Enter.
            sizeCombo->setCurrentText("12.5");
            emit sizeCombo->lineEdit()->editingFinished();
            CHECK(std::abs(brush.brushSize() - 12.5) < 1e-9,
                  "typing a decimal into the dropdown sets the tool's size");
        }
    }

    SECTION("Line / Curve tool: draw, commit, cancel");
    {
        // Drawing lays a straight segment but does NOT paint yet — the tool waits
        // in edit mode with control nubs. Committing (here via deactivate, as a
        // tool switch would) bakes it in. Escape / no-commit paints nothing.
        Document doc(64, 64);
        doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::red);
        CanvasWidget canvas;
        canvas.setDocument(&doc);

        LineTool line;
        QImage before = doc.activeLayer()->image().copy();
        strokeTool(&line, canvas, QPointF(8, 8), QPointF(56, 56));
        CHECK(!imagesDiffer(before, doc.activeLayer()->image()),
              "drawing a line does not paint until committed (edit mode)");
        line.deactivate(canvas);   // switching away commits
        CHECK(imagesDiffer(before, doc.activeLayer()->image()),
              "committing the line paints it into the layer");

        // A fresh line that is never committed leaves the layer untouched.
        Document doc2(64, 64);
        doc2.activeLayer()->clear(Qt::white);
        CanvasWidget canvas2;
        canvas2.setDocument(&doc2);
        LineTool line2;
        QImage before2 = doc2.activeLayer()->image().copy();
        strokeTool(&line2, canvas2, QPointF(8, 8), QPointF(56, 56));
        QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        line2.keyPressEvent(&esc, canvas2);
        line2.deactivate(canvas2);
        CHECK(!imagesDiffer(before2, doc2.activeLayer()->image()),
              "Escape cancels the line — nothing is painted");
    }

    SECTION("Line / Curve tool: curve type shapes the committed path");
    {
        // Lay a line, bend an inner nub off-axis, then commit under each curve
        // type. Straight (polyline) and Bézier build different shapes from the
        // same nubs, and the Cubic Spline is verified to pass through the moved
        // interior nub (interpolating), unlike the Bézier which only approaches it.
        auto committedFor = [](LineTool::CurveType type, QPointF bendTo) {
            Document doc(64, 64);
            doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::red);
            CanvasWidget canvas;
            canvas.setDocument(&doc);
            LineTool line;
            line.setCurveType(type);
            // Drag out the straight base line; the tool drops into edit mode.
            strokeTool(&line, canvas, QPointF(8, 8), QPointF(56, 56));
            // Grab inner nub #1 (at canvas (24,24)) and drag it off the axis.
            const QPointF nubW = canvas.canvasToWidget(QPointF(24, 24));
            QMouseEvent gp(QEvent::MouseButtonPress, nubW, nubW,
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            line.mousePressEvent(canvas.widgetToCanvas(nubW), &gp, canvas);
            QMouseEvent gm = moveEv(bendTo);
            line.mouseMoveEvent(bendTo, &gm, canvas);
            QMouseEvent gr = releaseEv(bendTo);
            line.mouseReleaseEvent(bendTo, &gr, canvas);
            line.deactivate(canvas);   // commit
            return doc.activeLayer()->image().copy();
        };

        const QPointF bend(24, 4);   // pull nub #1 well above the diagonal
        QImage straight = committedFor(LineTool::CurveType::Straight, bend);
        QImage bezier   = committedFor(LineTool::CurveType::Bezier, bend);
        QImage spline   = committedFor(LineTool::CurveType::CubicSpline, bend);

        CHECK(imagesDiffer(straight, bezier),
              "Straight and Bézier produce different rasters from the same nubs");
        CHECK(imagesDiffer(spline, bezier),
              "Cubic Spline differs from Bézier for the same nubs");

        // The spline interpolates the moved nub: a painted pixel sits right on it.
        auto paintedNear = [](const QImage &img, QPointF p, int r) {
            for (int dy = -r; dy <= r; ++dy)
                for (int dx = -r; dx <= r; ++dx) {
                    const int x = int(p.x()) + dx, y = int(p.y()) + dy;
                    if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
                    if (img.pixelColor(x, y) != QColor(Qt::white)) return true;
                }
            return false;
        };
        CHECK(paintedNear(spline, bend, 2),
              "Cubic Spline passes through the interior nub");
    }

    SECTION("Shape tool: editable before commit (Paint.NET)");
    {
        // Like the Line/Curve tool, dragging a shape lays out its bounding box but
        // does NOT paint — the shape stays live for editing until committed.
        Document doc(64, 64);
        doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::red);
        doc.setSecondaryColor(Qt::red);
        CanvasWidget canvas;
        canvas.setDocument(&doc);

        ShapeTool sh;
        sh.setShapeFill(ShapeFill::Filled);
        QImage before = doc.activeLayer()->image().copy();
        strokeTool(&sh, canvas, QPointF(8, 8), QPointF(56, 56));
        CHECK(!imagesDiffer(before, doc.activeLayer()->image()),
              "drawing a shape does not paint until committed (edit mode)");
        sh.deactivate(canvas);   // switching away commits
        CHECK(imagesDiffer(before, doc.activeLayer()->image()),
              "committing the shape paints it into the layer");

        // A fresh shape cancelled with Escape leaves the layer untouched.
        Document doc2(64, 64);
        doc2.activeLayer()->clear(Qt::white);
        CanvasWidget canvas2;
        canvas2.setDocument(&doc2);
        ShapeTool sh2;
        sh2.setShapeFill(ShapeFill::Filled);
        QImage before2 = doc2.activeLayer()->image().copy();
        strokeTool(&sh2, canvas2, QPointF(8, 8), QPointF(56, 56));
        QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        sh2.keyPressEvent(&esc, canvas2);
        sh2.deactivate(canvas2);
        CHECK(!imagesDiffer(before2, doc2.activeLayer()->image()),
              "Escape cancels the shape — nothing is painted");

        // Enter commits, exactly like the Line/Curve tool.
        Document doc3(64, 64);
        doc3.activeLayer()->clear(Qt::white);
        doc3.setPrimaryColor(Qt::blue);
        doc3.setSecondaryColor(Qt::blue);
        CanvasWidget canvas3;
        canvas3.setDocument(&doc3);
        ShapeTool sh3;
        sh3.setShapeFill(ShapeFill::Filled);
        QImage before3 = doc3.activeLayer()->image().copy();
        strokeTool(&sh3, canvas3, QPointF(8, 8), QPointF(56, 56));
        QKeyEvent ent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        CHECK(sh3.wantsCommitKey(Qt::Key_Return), "shape claims Enter while editing");
        sh3.keyPressEvent(&ent, canvas3);
        CHECK(imagesDiffer(before3, doc3.activeLayer()->image()),
              "Enter commits the shape");
    }

    SECTION("Shape catalogue: expanded set paints (issue #17)");
    {
        // Every catalogue entry must produce a non-empty raster once committed.
        const ShapeType kinds[] = {
            ShapeType::Rectangle, ShapeType::RoundedRectangle, ShapeType::Ellipse,
            ShapeType::Diamond, ShapeType::Triangle, ShapeType::Trapezoid,
            ShapeType::Parallelogram, ShapeType::RightTriangle, ShapeType::Pentagon,
            ShapeType::Hexagon, ShapeType::Heptagon, ShapeType::Octagon,
            ShapeType::Star3, ShapeType::Star4, ShapeType::Star, ShapeType::Star6,
            ShapeType::Arrow, ShapeType::Chevron, ShapeType::SpeechBalloon,
            ShapeType::Cloud, ShapeType::Heart, ShapeType::Lightning,
            ShapeType::Cross, ShapeType::Check,
        };
        CHECK(std::size(kinds) == 24, "the catalogue has 24 shapes");
        // The enum's numeric range must cover exactly this set, in order.
        CHECK(static_cast<int>(ShapeType::Check) == 23, "Check is the last enum value");
        for (ShapeType k : kinds) {
            Document doc(80, 80);
            doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::black);
            doc.setSecondaryColor(Qt::black);
            CanvasWidget canvas; canvas.setDocument(&doc);
            ShapeTool sh;
            sh.setShapeType(k);
            sh.setShapeFill(ShapeFill::Both);
            QImage before = doc.activeLayer()->image().copy();
            strokeTool(&sh, canvas, QPointF(10, 10), QPointF(70, 70));
            CHECK(!imagesDiffer(before, doc.activeLayer()->image()),
                  "shape is not painted before commit");
            sh.deactivate(canvas);
            CHECK(imagesDiffer(before, doc.activeLayer()->image()),
                  "committed shape paints pixels");
        }
    }

    SECTION("Shape tool: rotate + move within the edit session");
    {
        // Rotating the live shape changes the committed raster vs. the un-rotated
        // one; moving it shifts the painted pixels. Both happen without a fresh
        // drag, proving the shape stays editable.
        auto paintedBBox = [](double angleDeg, QPointF nudge) {
            Document doc(100, 100);
            doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::black);
            doc.setSecondaryColor(Qt::black);
            CanvasWidget canvas; canvas.setDocument(&doc);
            ShapeTool sh;
            sh.setShapeType(ShapeType::Rectangle);
            sh.setShapeFill(ShapeFill::Filled);
            // Lay out a wide, short rect so rotation is visible.
            QMouseEvent p(QEvent::MouseButtonPress, QPointF(20, 40), QPointF(20, 40),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            sh.mousePressEvent(QPointF(20, 40), &p, canvas);
            QMouseEvent m(QEvent::MouseMove, QPointF(80, 60), QPointF(80, 60),
                          Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            sh.mouseMoveEvent(QPointF(80, 60), &m, canvas);
            QMouseEvent r(QEvent::MouseButtonRelease, QPointF(80, 60), QPointF(80, 60),
                          Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            sh.mouseReleaseEvent(QPointF(80, 60), &r, canvas);
            // Rotate by grabbing the rotation handle (widget space).
            if (angleDeg != 0.0) {
                const QPointF centre(50, 50);
                const QPointF rotHandle = canvas.canvasToWidget(QPointF(50, 40 - 24));
                QMouseEvent rp(QEvent::MouseButtonPress, rotHandle, rotHandle,
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                sh.mousePressEvent(canvas.widgetToCanvas(rotHandle), &rp, canvas);
                // Drag the handle to ~angleDeg around the centre.
                const double rad = (angleDeg - 90.0) * M_PI / 180.0;
                const QPointF target = centre + QPointF(std::cos(rad), std::sin(rad)) * 40.0;
                const QPointF tW = canvas.canvasToWidget(target);
                QMouseEvent rm(QEvent::MouseMove, tW, tW, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                sh.mouseMoveEvent(target, &rm, canvas);
                QMouseEvent ru(QEvent::MouseButtonRelease, tW, tW, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                sh.mouseReleaseEvent(target, &ru, canvas);
            }
            // Nudge with arrow keys.
            for (int i = 0; i < int(nudge.x()); ++i) {
                QKeyEvent k(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
                sh.keyPressEvent(&k, canvas);
            }
            for (int i = 0; i < int(nudge.y()); ++i) {
                QKeyEvent k(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
                sh.keyPressEvent(&k, canvas);
            }
            sh.deactivate(canvas);
            const QImage &img = doc.activeLayer()->image();
            int minx = 999, miny = 999, maxx = -1, maxy = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x)
                    if (qGray(img.pixel(x, y)) < 128) {
                        minx = qMin(minx, x); maxx = qMax(maxx, x);
                        miny = qMin(miny, y); maxy = qMax(maxy, y);
                    }
            return std::make_tuple(minx, miny, maxx, maxy);
        };

        auto [ax0, ay0, ax1, ay1] = paintedBBox(0.0, QPointF(0, 0));
        auto [bx0, by0, bx1, by1] = paintedBBox(90.0, QPointF(0, 0));
        // The un-rotated rect is wide & short; rotated 90° it becomes tall & narrow.
        const int wA = ax1 - ax0, hA = ay1 - ay0;
        const int wB = bx1 - bx0, hB = by1 - by0;
        CHECK(wA > hA, "un-rotated rect is wider than tall");
        CHECK(hB > wB, "after a 90° rotation the rect is taller than wide");

        auto [nx0, ny0, nx1, ny1] = paintedBBox(0.0, QPointF(10, 10));
        CHECK(nx0 > ax0 && ny0 > ay0, "arrow-key nudges move the shape before commit");
    }

    SECTION("Fill Style: solid + hatch patterns");
    {
        // Paint.NET's Fill Style is Solid Color plus the GDI+ hatch set (~53).
        CHECK(Hatch::count() >= 54, "at least 54 fill styles (Solid + 53 hatches)");
        CHECK(Hatch::name(0) == QStringLiteral("Solid Color"), "index 0 is Solid Color");

        auto fillWith = [](int style) {
            QImage img(16, 16, QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);
            QPainter p(&img);
            p.setPen(Qt::NoPen);
            p.setBrush(Hatch::brush(style, Qt::black, Qt::white));
            p.drawRect(0, 0, 16, 16);
            p.end();
            int black = 0, white = 0;
            for (int y = 0; y < 16; ++y)
                for (int x = 0; x < 16; ++x) {
                    QRgb c = img.pixel(x, y);
                    if (qRed(c) < 40) ++black; else if (qRed(c) > 215) ++white;
                }
            return std::make_pair(black, white);
        };

        auto [sb, sw] = fillWith(0);
        CHECK(sb > 200 && sw == 0, "Solid style fills uniformly with the foreground");
        // A hatch has BOTH foreground and background pixels — real structure.
        auto [hb, hw] = fillWith(5);   // Cross
        CHECK(hb > 0 && hw > 0, "a hatch pattern paints both fg and bg pixels");
        auto [pb, pw] = fillWith(12);  // 50%
        CHECK(pb > 40 && pw > 40, "the 50% pattern is roughly half foreground");

        // End to end: a filled shape with a hatch fill style paints a pattern,
        // not a flat colour.
        Document doc(64, 64);
        doc.activeLayer()->clear(Qt::white);
        doc.setPrimaryColor(Qt::black);
        doc.setSecondaryColor(Qt::white);
        CanvasWidget canvas; canvas.setDocument(&doc);
        ShapeTool sh;
        sh.setShapeFill(ShapeFill::Filled);
        sh.setFillStyle(5);            // Cross hatch
        strokeTool(&sh, canvas, QPointF(8, 8), QPointF(56, 56));
        sh.deactivate(canvas);         // commit the editable shape into the layer
        int fg = 0, bg = 0;
        const QImage &out = doc.activeLayer()->image();
        for (int y = 20; y < 44; ++y)
            for (int x = 20; x < 44; ++x) {
                QRgb c = out.pixel(x, y);
                if (qRed(c) < 40) ++fg; else if (qRed(c) > 215) ++bg;
            }
        CHECK(fg > 0 && bg > 0, "a hatch-filled shape shows the pattern (fg + bg inside)");
    }

    SECTION("Marching ants trace only the perimeter (issue #15)");
    {
        // A non-rectangular selection's QRegion is many scanline rects. Stroking
        // each one filled the interior with static; the fix strokes only the
        // merged outline. Render the canvas and check the selection interior is
        // clean (no dash pixels away from the boundary).
        Document doc(80, 80);
        doc.activeLayer()->clear(Qt::white);
        QPainterPath blob;
        blob.addEllipse(QRectF(10, 10, 60, 60));
        doc.selection().selectPath(blob, SelectionMode::Replace);

        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(120, 120);
        canvas.show();
        QCoreApplication::processEvents();

        const QImage shot = canvas.grab().toImage();
        // Sample a small patch at the ellipse centre, well inside the boundary.
        const QPointF c = canvas.canvasToWidget(QPointF(40, 40));
        int dark = 0, total = 0;
        for (int dy = -6; dy <= 6; ++dy)
            for (int dx = -6; dx <= 6; ++dx) {
                const QPoint p = c.toPoint() + QPoint(dx, dy);
                if (!shot.rect().contains(p)) continue;
                ++total;
                if (qGray(shot.pixel(p)) < 128) ++dark;
            }
        // With the bug the interior is a dense dash grid; with the fix it's the
        // clean white layer. Allow a couple of stray antialiased pixels.
        CHECK(total > 0 && dark <= 2, "selection interior is clean, not filled with static");
    }

    SECTION("Brush pressure varies dab size, not opacity (issue #16)");
    {
        auto paintedArea = [](double pressure, bool sensitivity) {
            Document doc(80, 80);
            doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::black);
            CanvasWidget canvas; canvas.setDocument(&doc);
            BrushTool brush;
            brush.setBrushSize(24);
            brush.setPressureSensitivity(sensitivity);
            brush.setPressure(pressure);
            strokeTool(&brush, canvas, QPointF(15, 40), QPointF(65, 40));
            const QImage &img = doc.activeLayer()->image();
            int painted = 0, darkCore = 0;
            for (int y = 0; y < 80; ++y)
                for (int x = 0; x < 80; ++x) {
                    int g = qGray(img.pixel(x, y));
                    if (g < 250) ++painted;       // any mark
                    if (g < 40) ++darkCore;       // fully-opaque black
                }
            return std::make_pair(painted, darkCore);
        };

        auto [fullArea, fullCore] = paintedArea(1.0, true);
        auto [lightArea, lightCore] = paintedArea(0.4, true);
        CHECK(lightArea > 0 && lightArea < fullArea * 0.7,
              "lower pressure paints a visibly thinner stroke");
        // Not opacity: the pixels that ARE painted stay fully opaque black.
        CHECK(lightCore > 0, "low-pressure stroke is still solid (opacity unchanged)");

        // Sensitivity off: full size regardless of pressure.
        auto [offArea, offCore] = paintedArea(0.4, false);
        CHECK(offArea > fullArea * 0.9, "pressure OFF stamps the full brush size");

        // The same size-not-opacity model now applies to the Eraser: lower
        // pressure clears a thinner track.
        auto erasedArea = [](double pressure) {
            Document doc(80, 80);
            doc.activeLayer()->clear(Qt::black);   // opaque, so erasing shows
            CanvasWidget canvas; canvas.setDocument(&doc);
            EraserTool er;
            er.setBrushSize(24);
            er.setPressureSensitivity(true);
            er.setPressure(pressure);
            strokeTool(&er, canvas, QPointF(15, 40), QPointF(65, 40));
            const QImage &img = doc.activeLayer()->image();
            int cleared = 0;
            for (int y = 0; y < 80; ++y)
                for (int x = 0; x < 80; ++x)
                    if (qAlpha(img.pixel(x, y)) < 128) ++cleared;
            return cleared;
        };
        const int erFull = erasedArea(1.0), erLight = erasedArea(0.4);
        CHECK(erLight > 0 && erLight < erFull * 0.7,
              "eraser: lower pressure clears a thinner track (size, not opacity)");
    }

    SECTION("Shape tool: Shift constrains 1:1, Alt draws from centre, corner size");
    {
        // Strokes a filled rectangle from a->b with the given modifiers held, then
        // returns the bounding box of the painted (black) pixels.
        auto bbox = [](Qt::KeyboardModifiers mods, QPointF a, QPointF b) {
            Document doc(120, 120);
            doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::black);
            doc.setSecondaryColor(Qt::black);
            CanvasWidget canvas; canvas.setDocument(&doc);
            ShapeTool sh;
            sh.setShapeType(ShapeType::Rectangle);
            sh.setShapeFill(ShapeFill::Filled);
            QMouseEvent e1(QEvent::MouseButtonPress, a, a, Qt::LeftButton, Qt::LeftButton, mods);
            sh.mousePressEvent(a, &e1, canvas);
            QMouseEvent e2(QEvent::MouseMove, b, b, Qt::NoButton, Qt::LeftButton, mods);
            sh.mouseMoveEvent(b, &e2, canvas);
            QMouseEvent e3(QEvent::MouseButtonRelease, b, b, Qt::LeftButton, Qt::NoButton, mods);
            sh.mouseReleaseEvent(b, &e3, canvas);
            sh.deactivate(canvas);   // commit the editable shape into the layer
            const QImage &img = doc.activeLayer()->image();
            int minx = 999, miny = 999, maxx = -1, maxy = -1;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x)
                    if (qGray(img.pixel(x, y)) < 128) {
                        minx = qMin(minx, x); maxx = qMax(maxx, x);
                        miny = qMin(miny, y); maxy = qMax(maxy, y);
                    }
            return std::make_tuple(minx, miny, maxx, maxy);
        };
        auto approx = [](int v, int target) { return std::abs(v - target) <= 2; };

        // Plain drag: 40 wide, 20 tall from (20,20).
        {
            auto [x0, y0, x1, y1] = bbox(Qt::NoModifier, QPointF(20, 20), QPointF(60, 40));
            CHECK(approx(x0,20) && approx(y0,20) && approx(x1,60) && approx(y1,40),
                  "plain drag draws the raw drag rectangle");
        }
        // Shift: constrain to a square using the larger extent (40), toward cursor.
        {
            auto [x0, y0, x1, y1] = bbox(Qt::ShiftModifier, QPointF(20, 20), QPointF(60, 40));
            CHECK(approx(x0,20) && approx(y0,20) && approx(x1,60) && approx(y1,60),
                  "Shift forces a 1:1 square growing toward the cursor");
        }
        // Alt: start point is the centre; extend equally to both sides.
        {
            auto [x0, y0, x1, y1] = bbox(Qt::AltModifier, QPointF(60, 60), QPointF(80, 70));
            CHECK(approx(x0,40) && approx(y0,50) && approx(x1,80) && approx(y1,70),
                  "Alt centres the shape on the start point");
        }
        // Shift+Alt: centred square (extent 20 on the larger axis).
        {
            auto [x0, y0, x1, y1] = bbox(Qt::ShiftModifier | Qt::AltModifier,
                                         QPointF(60, 60), QPointF(80, 70));
            CHECK(approx(x0,40) && approx(y0,40) && approx(x1,80) && approx(y1,80),
                  "Shift+Alt draws a centred 1:1 square");
        }

        // Corner size clamps to 0..200 and is honoured by the rounded rectangle:
        // a larger radius rounds away more of the corner pixels.
        ShapeTool cs;
        cs.setCornerSize(-5);  CHECK(cs.cornerSize() == 0,   "corner size clamps at 0");
        cs.setCornerSize(999); CHECK(cs.cornerSize() == 200, "corner size clamps at 200");
        auto cornerFilled = [](int radius) {
            Document doc(80, 80);
            doc.activeLayer()->clear(Qt::white);
            doc.setPrimaryColor(Qt::black);
            doc.setSecondaryColor(Qt::black);
            CanvasWidget canvas; canvas.setDocument(&doc);
            ShapeTool sh;
            sh.setShapeType(ShapeType::RoundedRectangle);
            sh.setShapeFill(ShapeFill::Filled);
            sh.setCornerSize(radius);
            strokeTool(&sh, canvas, QPointF(10, 10), QPointF(70, 70));
            sh.deactivate(canvas);   // commit the editable shape into the layer
            // Is the top-left corner pixel (just inside the bbox) painted?
            return qGray(doc.activeLayer()->image().pixel(12, 12)) < 128;
        };
        CHECK(cornerFilled(0),  "corner size 0 leaves a sharp (filled) corner");
        CHECK(!cornerFilled(30), "a large corner size rounds the corner away");
    }

    SECTION("WebP import/export round-trips when the codec is present (issue #19)");
    {
        // WebP is offered in Open/Save; it works only if Qt's image-format plugin
        // is installed (the packaging now depends on qt6-image-formats-plugins).
        // Skip cleanly where the codec is absent (e.g. a bare dev box) so this
        // isn't a false failure, but exercise the real round-trip in CI where the
        // plugin is installed.
        const bool canWrite = QImageWriter::supportedImageFormats().contains("webp");
        const bool canRead  = QImageReader::supportedImageFormats().contains("webp");
        if (!canWrite || !canRead) {
            printf("    (webp codec not installed here — skipped; CI has the plugin)\n");
        } else {
            QImage src(40, 30, QImage::Format_ARGB32);
            for (int y = 0; y < 30; ++y)
                for (int x = 0; x < 40; ++x)
                    src.setPixelColor(x, y, QColor(x * 6, y * 8, 120));
            QTemporaryFile f("XXXXXX.webp");
            f.setAutoRemove(true);
            CHECK(f.open(), "temp .webp file opens");
            const QString path = f.fileName();
            QImageWriter w(path, "webp");
            w.setQuality(100);   // near-lossless
            CHECK(w.write(src), "QImageWriter writes a .webp file");
            QImage back(path);
            CHECK(!back.isNull() && back.size() == QSize(40, 30),
                  "the .webp reloads at the right size");
            const QColor c = back.convertToFormat(QImage::Format_ARGB32).pixelColor(20, 15);
            const QColor want(120, 120, 120);
            CHECK(std::abs(c.red() - want.red()) < 24 && std::abs(c.green() - want.green()) < 24
                  && std::abs(c.blue() - want.blue()) < 24,
                  "webp colours round-trip within tolerance");
        }
    }

    SECTION("Save Configuration: JPEG quality changes size and is remembered (Paint.NET)");
    {
        // A photo-like gradient so JPEG quality actually moves the file size.
        Document doc(64, 64);
        QImage img(64, 64, QImage::Format_ARGB32_Premultiplied);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                img.setPixelColor(x, y, QColor((x * 7) % 256, (y * 11) % 256, (x * y) % 256));
        doc.activeLayer()->setImage(img);

        QTemporaryDir dir;
        CHECK(dir.isValid(), "temp dir for JPEG saves");
        const QString lowPath  = dir.filePath("low.jpg");
        const QString highPath = dir.filePath("high.jpg");

        CHECK(doc.save(lowPath, 10),  "save JPEG at quality 10");
        CHECK(doc.save(highPath, 95), "save JPEG at quality 95");
        const qint64 lowSize  = QFileInfo(lowPath).size();
        const qint64 highSize = QFileInfo(highPath).size();
        CHECK(lowSize > 0 && highSize > 0, "both JPEGs are written");
        CHECK(lowSize < highSize, "lower quality yields a smaller JPEG");

        // A plain re-save (quality == -1) must reuse the last chosen quality, so a
        // Ctrl+S after Save As keeps the setting instead of reverting to default.
        const QString reuse = dir.filePath("reuse.jpg");
        CHECK(doc.save(reuse), "re-save without an explicit quality");
        const qint64 reuseSize = QFileInfo(reuse).size();
        CHECK(std::abs(reuseSize - highSize) < highSize / 5,
              "re-save reuses the remembered quality (≈ last size)");

        // The dialog only offers itself for lossy formats.
        CHECK(SaveConfigDialog::supportsQuality("jpg")
              && SaveConfigDialog::supportsQuality("webp")
              && !SaveConfigDialog::supportsQuality("png"),
              "quality dialog is offered for JPEG/WebP, not PNG");
    }

    // ---------- RESULT ----------
    printf("\n=====================================\n");
    printf("PASSED: %d   FAILED: %d\n", g_pass, g_fail);
    printf("%s\n", g_fail == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return g_fail == 0 ? 0 : 1;
}
