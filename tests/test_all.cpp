// Comprehensive headless functional test for PaintDali.
// Exercises the real code paths: tools, effects, adjustments, layers,
// selection, undo/redo, save/load, i18n and theming.
#include <QApplication>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>
#include <QDir>
#include <QFile>
#include <cstdio>
#include <functional>
#include <climits>

#include "core/document.h"
#include "core/layer.h"
#include "core/selection.h"
#include "core/history.h"
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

// =====================================================================
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    // ---------- TOOLS ----------
    SECTION("Tools draw into the active layer");
    {
        auto testDraw = [](const char *name, Tool *tool, bool useTwoPoints = true) {
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
            bool changed = imagesDiffer(before, doc.activeLayer()->image());
            CHECK(changed, name);
        };
        BrushTool brush;        testDraw("Brush modifies pixels", &brush);
        PencilTool pencil;      testDraw("Pencil modifies pixels", &pencil);
        FillTool fill;          testDraw("Fill modifies pixels", &fill, false);
        LineTool line;          testDraw("Line modifies pixels", &line);
        ShapeTool shape;        testDraw("Shape modifies pixels", &shape);
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
        QString png = "/tmp/_pdtest.png", jpg = "/tmp/_pdtest.jpg";
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
    printf("\n--- Text tool captures shortcut letters ---\n");
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

    // ---------- NEW LAYERS ARE TRANSPARENT ----------
    printf("\n--- New layers are transparent ---\n");
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

    // ---------- LAYER MOVE / FLIP / IMPORT ----------
    printf("\n--- Layer move / flip / import ---\n");
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
    printf("\n--- Selection modes and modify ---\n");
    {
        auto area = [](const Selection &s, int w, int h) {
            long n = 0;
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                    if (s.isSelected(x, y)) ++n;
            return n;
        };
        // The shared modifier→mode helper every selection tool now uses.
        CHECK(selectionModeFor(Qt::NoModifier) == SelectionMode::Replace, "no modifier = replace");
        CHECK(selectionModeFor(Qt::ShiftModifier) == SelectionMode::Add, "shift = add");
        CHECK(selectionModeFor(Qt::ControlModifier) == SelectionMode::Subtract, "ctrl = subtract");
        CHECK(selectionModeFor(Qt::ShiftModifier | Qt::ControlModifier) == SelectionMode::Intersect,
              "shift+ctrl = intersect");

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
    printf("\n--- Fill selection ---\n");
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
    printf("\n--- Text tool: paste ---\n");
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
    printf("\n--- Move tool: resize selected pixels ---\n");
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
    printf("\n--- Move tool: undo/redo a resize ---\n");
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
    printf("\n--- Move tool acts on the selected layer ---\n");
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

    // ---------- MOVE SELECTION TOOL ----------
    printf("\n--- Move Selection tool ---\n");
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

    // ---------- CANVAS VIEW CENTRING ----------
    printf("\n--- Canvas view centring ---\n");
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

    // ---------- RESULT ----------
    printf("\n=====================================\n");
    printf("PASSED: %d   FAILED: %d\n", g_pass, g_fail);
    printf("%s\n", g_fail == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return g_fail == 0 ? 0 : 1;
}
