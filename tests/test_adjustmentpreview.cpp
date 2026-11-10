#include "core/adjustmentpreview.h"
#include "canvas/canvaswidget.h"
#include "adjustments/curves.h"
#include "dialogs/curvesdialog.h"
#include "dialogs/previewdialog.h"
#include "dialogs/huesaturationdialog.h"
#include "adjustments/huesaturation.h"
#include "adjustments/levels.h"
#include "adjustments/sepia.h"
#include "adjustments/posterize.h"
#include "adjustments/threshold.h"
#include "adjustments/colorbalance.h"
#include "adjustments/exposure.h"
#include "adjustments/highlightsshadows.h"
#include "adjustments/temperaturetint.h"
#include "adjustments/brightnesscontrast.h"
#include <QSlider>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QMouseEvent>
#include <QPushButton>
#include "mainwindow.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>
#include <QThreadPool>
#include <iostream>
#include <cstdlib>

static void check(bool value, const char *message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
static bool until(const std::function<bool()> &predicate) {
    QElapsedTimer timer; timer.start();
    while (!predicate() && timer.elapsed() < 5000) {
        QApplication::processEvents(); QThread::msleep(1);
    }
    return predicate();
}
static ImageOperation curve(int lift) {
    return [lift](const QImage &image) {
        Curves c; c.setControlPoints({{0, double(lift)}, {255,255}}); return c.apply(image);
    };
}
static QImage sample(QSize size = {8,8}) {
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(80,120,160,128));
    image.setPixel(0,0,0);
    return image;
}
int main(int argc, char **argv) {
    QTemporaryDir settings;
    check(settings.isValid(),"temporary settings directory");
    for (const char *name : {"XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME", "XDG_RUNTIME_DIR"}) {
        const QString path = settings.path() + '/' + QString::fromLatin1(name);
        QDir().mkpath(path);
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        qputenv(name,path.toUtf8());
    }
    QApplication app(argc, argv);
    Document doc(8,8);
    doc.layerAt(0)->setImage(sample());
    CanvasWidget canvas; canvas.setDocument(&doc); canvas.resize(160,160);
    canvas.show(); canvas.zoomToFit(); QApplication::processEvents();
    const QImage canvasBefore = canvas.grab().toImage();
    const QImage original = doc.layerAt(0)->image();
    const auto initial = doc.history().currentIndex();
    const bool modified = doc.isModified();
    {
        AdjustmentPreview preview(&doc,&canvas,{0});
        int frames = 0; QImage displayed;
        QObject::connect(&preview,&AdjustmentPreview::previewReady,&app,[&](const QImage &image) {
            check(QThread::currentThread() == app.thread(),"GUI-thread delivery");
            displayed = image; ++frames;
        });
        preview.request(curve(30));
        check(until([&]{return frames == 1;}),"first live frame");
        check(displayed == curve(30)(original).convertToFormat(original.format()),"preview colour and alpha");
        preview.request(curve(70));
        check(until([&]{return frames == 2;}),"second live frame");
        check(displayed == curve(70)(original).convertToFormat(original.format()),"non-cumulative processing");
        check(doc.layerAt(0)->image() == original,"preview leaves document unchanged");
        check(canvas.grab().toImage() != canvasBefore,"main canvas displays override");
        preview.cancel();
        check(canvas.grab().toImage() == canvasBefore,"Cancel clears canvas override");
        preview.request(curve(90));
        QApplication::processEvents();
        check(frames == 2,"cancel rejects requests");
        check(doc.history().currentIndex() == initial && doc.isModified() == modified,"cancel preserves state");
    }
    // Exercise the actual curve editor's add/move/remove/reset notifications.
    {
        CurvesDialog dialog;
        AdjustmentPreview session(&doc,&canvas,{0});
        int frames = 0;
        QObject::connect(&session,&AdjustmentPreview::previewReady,&app,[&](const QImage &){++frames;});
        QObject::connect(&dialog,&CurvesDialog::curveChanged,&session,[&]() {
            const auto points = dialog.controlPoints();
            session.request([points](const QImage &im) { Curves c; c.setControlPoints(points); return c.apply(im); });
        });
        QWidget *editor = nullptr;
        for (auto *widget : dialog.findChildren<QWidget *>())
            if (widget->size() == QSize(256,256)) editor = widget;
        check(editor,"curve editor found");
        auto mouse = [&](QEvent::Type type, QPointF point, Qt::MouseButton button, Qt::MouseButtons buttons) {
            QMouseEvent event(type,point,point,button,buttons,Qt::NoModifier);
            QApplication::sendEvent(editor,&event);
        };
        mouse(QEvent::MouseButtonPress,{128,100},Qt::LeftButton,Qt::LeftButton);
        check(until([&]{return frames==1;}),"adding point previews");
        mouse(QEvent::MouseMove,{140,70},Qt::NoButton,Qt::LeftButton);
        mouse(QEvent::MouseButtonRelease,{140,70},Qt::LeftButton,Qt::NoButton);
        check(until([&]{return frames==2;}),"moving point previews");
        mouse(QEvent::MouseButtonPress,{140,70},Qt::RightButton,Qt::RightButton);
        check(until([&]{return frames==3;}),"removing point previews");
        check(dialog.controlPoints().size()==2,"point removed");
        for (auto *button : dialog.findChildren<QPushButton *>())
            if (button->text().contains("initialiser") || button->text() == "Reset") button->click();
        check(until([&]{return frames==4;}),"Reset previews");
        QObject::connect(&dialog,&QDialog::rejected,&session,&AdjustmentPreview::cancel);
        dialog.reject();
        check(doc.layerAt(0)->image()==original && doc.history().currentIndex()==initial,"dialog Cancel leaves document intact");
    }
    // Feathered selection in document coordinates on an offset, transparent layer.
    doc.layerAt(0)->setOffset({2,1});
    QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
    mask.scanLine(3)[4] = 128;
    mask.scanLine(3)[5] = 255;
    doc.selection().setMaskImage(mask);
    QImage expected = adjustedImage(original,{2,1},doc.selection(),curve(70));
    QImage full = curve(70)(original).convertToFormat(original.format());
    QRgb src = original.pixel(2,2), fx = full.pixel(2,2), half = expected.pixel(2,2);
    check(qRed(half) == (qRed(src)*127+qRed(fx)*128)/255,"feather coverage");
    check(expected.pixel(3,2) == full.pixel(3,2),"offset selected pixel");
    check(expected.pixel(1,2) == original.pixel(1,2),"unselected pixel unchanged");
    check(qAlpha(half) == qAlpha(src) && expected.pixel(0,0) == 0,"alpha preserved");
    {
        AdjustmentPreview preview(&doc,&canvas,{0});
        check(preview.commit(curve(70),"Curves"),"commit succeeds before preview completes");
        check(doc.layerAt(0)->image() == expected,"commit selection matches calculation");
    }
    check(doc.history().currentIndex() == initial+1,"one undo entry");
    doc.history().undo();
    check(doc.layerAt(0)->image() == original,"undo exact original");
    {
        AdjustmentPreview preview(&doc,&canvas,{0}); preview.request(curve(20)); preview.cancel();
    }
    check(doc.history().canRedo(),"Cancel preserves redo");
    doc.history().redo(); check(doc.layerAt(0)->image() == expected,"redo exact adjusted image");

    doc.selection().clear();
    doc.addLayer(sample(),"second");
    doc.layerAt(1)->setOpacity(0.6f); doc.layerAt(1)->setBlendMode(BlendMode::Multiply);
    auto before0 = doc.layerAt(0)->image(), before1 = doc.layerAt(1)->image();
    int historyBefore = doc.history().currentIndex();
    {
        AdjustmentPreview preview(&doc,&canvas,{0,1});
        QImage displayed;
        QObject::connect(&preview,&AdjustmentPreview::previewReady,&app,[&](const QImage &im){displayed=im;});
        preview.request(curve(40));
        check(until([&]{return !displayed.isNull();}),"multilayer preview");
        check(preview.commit(curve(40),"Curves"),"multilayer commit");
        check(displayed == doc.flattenVisible(),"preview matches actual blend/opacity/offset composite");
    }
    check(doc.history().currentIndex() == historyBefore+1,"all layers single undo");
    auto after0 = doc.layerAt(0)->image(), after1 = doc.layerAt(1)->image();
    doc.history().undo();
    check(doc.layerAt(0)->image()==before0 && doc.layerAt(1)->image()==before1,"all layers undone together");
    doc.history().redo();
    check(doc.layerAt(0)->image()==after0 && doc.layerAt(1)->image()==after1,"all layers redone together");
    {
        AdjustmentPreview preview(&doc,&canvas,{0,1});
        int count = doc.history().currentIndex();
        check(!preview.commit([](const QImage &im){return im;},"Identity"),"identity no commit");
        check(count == doc.history().currentIndex(),"identity no history");
    }
    // Running work must never deliver after cancel or session destruction.
    auto entered = std::make_shared<std::atomic_bool>(false);
    auto release = std::make_shared<std::atomic_bool>(false);
    ImageOperation blocked = [entered,release](const QImage &im) {
        entered->store(true);
        while (!release->load()) QThread::msleep(1);
        return curve(60)(im);
    };
    int late = 0;
    auto *preview = new AdjustmentPreview(&doc,&canvas,{0});
    QObject::connect(preview,&AdjustmentPreview::previewReady,&app,[&](const QImage &){++late;});
    preview->request(blocked);
    check(until([&]{return entered->load();}),"worker started");
    preview->cancel(); delete preview; release->store(true);
    QThreadPool::globalInstance()->waitForDone(); QApplication::processEvents();
    check(late == 0,"no callback after destruction");
    entered->store(false); release->store(false);
    {
        AdjustmentPreview session(&doc,&canvas,{0});
        QObject::connect(&session,&AdjustmentPreview::previewReady,&app,[&](const QImage &){++late;});
        session.request(blocked);
        check(until([&]{return entered->load();}),"cancel worker started");
        session.cancel(); release->store(true);
        QThreadPool::globalInstance()->waitForDone(); QApplication::processEvents();
        check(late==0,"no late frame after Cancel with session alive");
    }
    // Latest pending request wins, with older work displayed only before newer work.
    entered->store(false); release->store(false);
    {
        AdjustmentPreview session(&doc,&canvas,{0});
        QImage displayed; int frames = 0;
        QObject::connect(&session,&AdjustmentPreview::previewReady,&app,[&](const QImage &im){displayed=im;++frames;});
        session.request(blocked);
        check(until([&]{return entered->load();}),"blocked generation started");
        session.request(curve(10)); session.request(curve(90)); release->store(true);
        check(until([&]{return frames == 2;}),"latest pending frame delivered");
        std::vector<Layer> layers{*doc.layerAt(0),*doc.layerAt(1)};
        layers[0].setImage(curve(90)(after0));
        check(displayed == Document::compositeLayers(layers,doc.size()),"no stale replacement");
        doc.notifyContentChanged();
        check(!session.commit(curve(20),"invalid"),"external document change invalidates session");
    }
    // Actual MainWindow modal-dialog wiring: reject, immediate OK, grouped undo.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        check(view,"MainWindow canvas");
        auto *document = view->document();
        document->activeLayer()->setImage(sample());
        auto before = document->activeLayer()->image();
        int index = document->history().currentIndex();
        auto editDialog = [&window](bool accept) {
            QTimer::singleShot(0,&window,[&window,accept]() {
                auto *dialog = window.findChild<CurvesDialog *>();
                check(dialog,"MainWindow opens Curves dialog");
                QWidget *editor = nullptr;
                for (auto *widget : dialog->findChildren<QWidget *>())
                    if (widget->size() == QSize(256,256)) editor = widget;
                check(editor,"MainWindow curve editor");
                QMouseEvent event(QEvent::MouseButtonPress,QPointF(128,40),QPointF(128,40),
                                  Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
                QApplication::sendEvent(editor,&event);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,3)),"invoke Curves adjustment");
        };
        editDialog(false);
        check(document->activeLayer()->image()==before && document->history().currentIndex()==index,"MainWindow Cancel");
        editDialog(true);
        auto after = document->activeLayer()->image();
        check(after!=before && document->history().currentIndex()==index+1,"MainWindow immediate OK commits once");
        document->history().undo(); check(document->activeLayer()->image()==before,"MainWindow undo");
        document->history().redo(); check(document->activeLayer()->image()==after,"MainWindow redo");
    }
    // Brightness/Contrast uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Brightness/Contrast dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(30);
                spins[1]->setValue(15);
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Brightness updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Brightness preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({0,0}),"Brightness Reset synchronises controls");
                spins[0]->setValue(50);
                sliders[1]->setValue(10);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,0)),
                  "invoke Brightness/Contrast");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Brightness Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            BrightnessContrast adjustment; adjustment.setBrightness(50); adjustment.setContrast(10);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Brightness final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Brightness all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Brightness grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Brightness grouped redo");
    }
    // Hue/Saturation dialog: live canvas, original-based edits, neutral Reset, grouped history.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200); view->setShowRulers(false); view->setZoom(10); view->setPan({20,20});
        const auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](int mode) {
            QTimer::singleShot(0,&window,[&,mode]() {
                auto *dialog = window.findChild<HueSaturationDialog *>();
                check(dialog,"Hue/Saturation dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(90); spins[1]->setValue(50); spins[2]->setValue(20);
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Hue/Saturation updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Hue/Saturation preview leaves source images unchanged");
                for (auto *button : dialog->findChildren<QPushButton *>())
                    if (button->text()=="↺") button->click();
                if (mode == 2) {
                    dialog->accept(); // Reset followed by OK must be an identity operation.
                    return;
                }
                spins[0]->setValue(45); sliders[1]->setValue(125); sliders[2]->setValue(10);
                if (mode == 1) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,1)),
                  "invoke Hue/Saturation");
        };
        edit(0);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Hue/Saturation Cancel preserves state");
        edit(2);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0,"Hue/Saturation neutral Reset creates no undo entry");
        edit(1);
        auto operation = [](const QImage &image) {
            HueSaturation adjustment; adjustment.setHue(45); adjustment.setSaturation(25); adjustment.setLightness(10);
            return adjustment.apply(image);
        };
        const auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Hue/Saturation final parameters, mapping, feathering, offsets and alpha match original-based calculation");
        check(after0!=before0 && document->history().currentIndex()==1,"Hue/Saturation commits once across all layers");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Hue/Saturation grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Hue/Saturation grouped redo");
    }
    // Levels uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Levels dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(20);
                spins[1]->setValue(220);
                spins[2]->setValue(150);
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Levels updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Levels preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({0,255,100}),"Levels Reset synchronises controls");
                spins[0]->setValue(25);
                sliders[1]->setValue(230);
                spins[2]->setValue(180);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,2)),
                  "invoke Levels");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Levels Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            Levels adjustment; adjustment.setInputBlack(25); adjustment.setInputWhite(230); adjustment.setGamma(1.8);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Levels final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Levels all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Levels grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Levels grouped redo");
    }
    // Sepia uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Sepia dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                check(dialog->values()==QVector<int>({80}),"Sepia opens at default 80 percent");
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Sepia previews default intensity without a control change");
                const QRgb defaultPixel = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(20);
                spins[0]->setValue(40);
                check(until([&] { return view->grab().toImage().pixel(pixel) != defaultPixel; }),
                      "Sepia updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Sepia preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({80}),"Sepia Reset synchronises controls");
                spins[0]->setValue(60);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,5)),
                  "invoke Sepia");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Sepia Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            Sepia adjustment; adjustment.setIntensity(60);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Sepia final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Sepia all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Sepia grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Sepia grouped redo");
    }
    // Posterize uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Posterize dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                check(dialog->values()==QVector<int>({4}),"Posterize opens at default four levels");
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Posterize previews default levels without a control change");
                const QRgb defaultPixel = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(2);
                spins[0]->setValue(8);
                check(until([&] { return view->grab().toImage().pixel(pixel) != defaultPixel; }),
                      "Posterize updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Posterize preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({4}),"Posterize Reset synchronises controls");
                spins[0]->setValue(6);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,6)),
                  "invoke Posterize");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Posterize Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            Posterize adjustment; adjustment.setLevels(6);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Posterize final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Posterize all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Posterize grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Posterize grouped redo");
    }
    // Threshold uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Threshold dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                check(dialog->values()==QVector<int>({128}),"Threshold opens at default value 128");
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Threshold previews default threshold without a control change");
                const QRgb defaultPixel = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(255);
                spins[0]->setValue(0);
                check(until([&] { return view->grab().toImage().pixel(pixel) != defaultPixel; }),
                      "Threshold updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Threshold preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({128}),"Threshold Reset synchronises controls");
                spins[0]->setValue(100);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,14)),
                  "invoke Threshold");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Threshold Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            Threshold adjustment; adjustment.setThreshold(100);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Threshold final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Threshold all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Threshold grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Threshold grouped redo");
    }
    // Color Balance uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Color Balance dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(20);
                spins[1]->setValue(-30);
                spins[2]->setValue(40);
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Color Balance updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Color Balance preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({0,0,0}),"Color Balance Reset synchronises controls");
                spins[0]->setValue(25);
                sliders[1]->setValue(-20);
                spins[2]->setValue(35);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,8)),
                  "invoke Color Balance");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Color Balance Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            ColorBalance adjustment; adjustment.setCyanRed(25); adjustment.setMagentaGreen(-20); adjustment.setYellowBlue(35);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Color Balance final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Color Balance all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Color Balance grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Color Balance grouped redo");
    }
    // Exposure uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Exposure dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(20);
                spins[0]->setValue(-40);
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Exposure updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Exposure preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({0}),"Exposure Reset synchronises controls");
                spins[0]->setValue(25);

                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,10)),
                  "invoke Exposure");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Exposure Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            Exposure adjustment; adjustment.setExposure(25);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Exposure final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Exposure all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Exposure grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Exposure grouped redo");
    }
    // Highlights/Shadows uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Highlights/Shadows dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(20);
                spins[0]->setValue(-40);
                spins[1]->setValue(60);
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Highlights/Shadows updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Highlights/Shadows preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({0,0}),"Highlights/Shadows Reset synchronises controls");
                spins[0]->setValue(-25);
                sliders[1]->setValue(50);

                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,11)),
                  "invoke Highlights/Shadows");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Highlights/Shadows Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            HighlightsShadows adjustment; adjustment.setHighlights(-25); adjustment.setShadows(50);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Highlights/Shadows final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Highlights/Shadows all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Highlights/Shadows grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Highlights/Shadows grouped redo");
    }
    // Temperature/Tint uses the same session while retaining its thumbnail.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        document->layerAt(0)->setImage(sample());
        document->addLayer(sample(), "second");
        document->layerAt(1)->setOffset({1,0});
        document->setEditAllLayers(true);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(2)[3] = 128;
        document->selection().setMaskImage(mask);
        document->history().clear(); document->setModified(false);
        view->resize(200,200);
        view->setShowRulers(false);
        view->setZoom(10);
        view->setPan({20,20});
        auto before0 = document->layerAt(0)->image(), before1 = document->layerAt(1)->image();
        auto edit = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before0,before1]() {
                auto *dialog = window.findChild<PreviewDialog *>();
                check(dialog,"Temperature/Tint dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                auto spins = dialog->findChildren<QSpinBox *>();
                const QPoint pixel = view->canvasToWidget({3.5,2.5}).toPoint();
                const QRgb baseline = view->grab().toImage().pixel(pixel);
                sliders[0]->setValue(20);
                spins[0]->setValue(-40);
                spins[1]->setValue(60);
                check(until([&] { return view->grab().toImage().pixel(pixel) != baseline; }),
                      "Temperature/Tint updates main canvas while dialog is open");
                check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,
                      "Temperature/Tint preview leaves original data untouched");
                auto *buttons = dialog->findChild<QDialogButtonBox *>();
                buttons->button(QDialogButtonBox::Reset)->click();
                check(dialog->values()==QVector<int>({0,0}),"Temperature/Tint Reset synchronises controls");
                spins[0]->setValue(-25);
                sliders[1]->setValue(50);

                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"applyAdjustment",Qt::DirectConnection,Q_ARG(int,12)),
                  "invoke Temperature/Tint");
        };
        edit(false);
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1 &&
              document->history().currentIndex()==0 && !document->isModified(),"Temperature/Tint Cancel restores state");
        edit(true);
        auto operation = [](const QImage &image) {
            TemperatureTint adjustment; adjustment.setTemperature(-25); adjustment.setTint(50);
            return adjustment.apply(image);
        };
        auto after0 = document->layerAt(0)->image(), after1 = document->layerAt(1)->image();
        check(after0==adjustedImage(before0,{},document->selection(),operation) &&
              after1==adjustedImage(before1,{1,0},document->selection(),operation),
              "Temperature/Tint final values, feathering, offsets and transparency match original-based calculation");
        check(document->history().currentIndex()==1,"Temperature/Tint all layers commit one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before0 && document->layerAt(1)->image()==before1,"Temperature/Tint grouped undo");
        document->history().redo();
        check(document->layerAt(0)->image()==after0 && document->layerAt(1)->image()==after1,"Temperature/Tint grouped redo");
    }
    // Layers > Rotate / Zoom previews live on the canvas (issue #33), transforms
    // the whole layer even with a selection, and commits one undo entry.
    {
        MainWindow window;
        auto *view = window.findChild<CanvasWidget *>();
        auto *document = view->document();
        document->resize(8,8);
        QImage stripe(8,8,QImage::Format_ARGB32_Premultiplied);
        stripe.fill(Qt::white);
        for (int y = 0; y < 8; ++y) stripe.setPixel(0,y,qRgb(0,0,0));
        document->layerAt(0)->setImage(stripe);
        QImage mask(8,8,QImage::Format_Grayscale8); mask.fill(0);
        mask.scanLine(4)[4] = 255;
        document->selection().setMaskImage(mask);
        view->resize(200,200); view->zoomToFit(); QApplication::processEvents();
        const QImage before = document->layerAt(0)->image();
        auto run = [&](bool accept) {
            QTimer::singleShot(0,&window,[&window,view,document,accept,before]() {
                QDialog *dialog = nullptr;
                for (auto *d : window.findChildren<QDialog *>()) if (d->isVisible()) dialog = d;
                check(dialog,"Rotate / Zoom dialog opens");
                auto sliders = dialog->findChildren<QSlider *>();
                check(sliders.size()==2,"angle and scale sliders");
                const QImage baseline = view->grab().toImage();
                sliders[0]->setValue(90);
                check(until([&] { return view->grab().toImage() != baseline; }),
                      "Rotate / Zoom updates the canvas while the dialog is open");
                check(document->layerAt(0)->image()==before,"Rotate / Zoom preview leaves the layer untouched");
                sliders[1]->setValue(50);
                if (accept) dialog->accept(); else dialog->reject();
            });
            check(QMetaObject::invokeMethod(&window,"showRotateZoomLayerDialog",Qt::DirectConnection),
                  "invoke Rotate / Zoom");
        };
        const auto history0 = document->history().currentIndex();
        run(false);
        check(document->layerAt(0)->image()==before && document->history().currentIndex()==history0,
              "Rotate / Zoom Cancel restores state");
        run(true);
        check(document->layerAt(0)->image()==rotateZoomImage(before,90,0.5).convertToFormat(before.format()),
              "Rotate / Zoom commits the whole layer with the final values, ignoring the selection");
        check(document->history().currentIndex()==history0+1,"Rotate / Zoom adds one undo entry");
        document->history().undo();
        check(document->layerAt(0)->image()==before,"Rotate / Zoom undo");
    }
    std::cout << "Adjustment preview regressions passed\n";
}
