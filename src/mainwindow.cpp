#include "mainwindow.h"
#include "toolicons.h"
#include "i18n.h"
#include "theme.h"
#include "core/document.h"
#include "canvas/canvaswidget.h"
#include "panels/layerspanel.h"
#include "panels/colorspanel.h"
#include "panels/historypanel.h"
#include "panels/tooloptionspanel.h"
#include "panels/imagelistbar.h"
#include "dialogs/newdocumentdialog.h"
#include "dialogs/resizedialog.h"
#include "dialogs/canvassizedialog.h"
#include "dialogs/huesaturationdialog.h"
#include "dialogs/layerpropertiesdialog.h"
#include "dialogs/curvesdialog.h"
#include "dialogs/settingsdialog.h"
#include "dialogs/previewdialog.h"
#include "plugins/pluginmanager.h"
#include "plugins/plugineffect.h"
#include "tools/tool.h"
#include "tools/brushtool.h"
#include "tools/erasertool.h"
#include "tools/filltool.h"
#include "tools/colorpickertool.h"
#include "tools/selectiontool.h"
#include "tools/lassotool.h"
#include "tools/movetool.h"
#include "tools/moveselectiontool.h"
#include "tools/zoomtool.h"
#include "tools/pantool.h"
#include "tools/texttool.h"
#include "tools/shapetool.h"
#include "tools/linetool.h"
#include "tools/gradienttool.h"
#include "tools/clonestamptool.h"
#include "tools/magicwandtool.h"
#include "tools/penciltool.h"
#include "tools/recolortool.h"
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
#include "effects/dropshadoweffect.h"
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
#include "adjustments/invertalpha.h"

#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDesktopServices>
#include <QClipboard>
#include <QInputDialog>
#include <QScrollArea>
#include <QSlider>
#include <QLabel>
#include <QPainter>
#include <QGridLayout>
#include <QToolButton>
#include <QFileInfo>
#include <QSizePolicy>
#include <QTimer>
#include <QSettings>
#include <QCursor>
#include <QMenu>
#include <QScopedValueRollback>
#include <QPrinter>
#include <QPrintDialog>
#include <QWindow>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

// One-line tool description used in tooltips (defined further down).
QString toolDescription(ToolType t);

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(TR("Sans titre - paint.software 1.1"));
    resize(1400, 900);
    setMinimumSize(800, 600);
    setDockNestingEnabled(true);
    setDockOptions(QMainWindow::AnimatedDocks
                   | QMainWindow::AllowNestedDocks
                   | QMainWindow::AllowTabbedDocks
                   | QMainWindow::GroupedDragging);
    setAcceptDrops(true);

    // Default document (registered as the first entry of the image list)
    m_document = new Document(800, 600, this);
    m_documents.append(m_document);
    m_activeDocIndex = 0;

    // Canvas
    m_canvas = new CanvasWidget(this);
    m_canvas->setDocument(m_document);
    setCentralWidget(m_canvas);

    createTools();
    createToolsPanel();
    createDockPanels();
    createMenus();
    createToolBar();
    createStatusBar();
    createKeyboardShortcuts();
    loadUiState();

    // Connect signals
    connect(m_canvas, &CanvasWidget::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(m_canvas, &CanvasWidget::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(m_canvas, &CanvasWidget::deleteSelectionRequested, this, &MainWindow::deleteSelectionContents);
    connect(m_canvas, &CanvasWidget::selectionContextMenuRequested, this, &MainWindow::showSelectionContextMenu);
    connect(m_document, &Document::documentChanged, this, &MainWindow::updateTitle);
    connect(m_document, &Document::documentChanged, this, &MainWindow::updateImageList);
    connect(m_document, &Document::sizeChanged, this, [this](int w, int h) {
        m_sizeLabel->setText(QString("%1 × %2").arg(w).arg(h));
    });
    connect(&m_document->history(), &HistoryManager::historyChanged, this, [this]() {
        const bool canUndo = m_document->history().canUndo();
        const bool canRedo = m_document->history().canRedo();
        m_undoAction->setEnabled(canUndo);
        m_redoAction->setEnabled(canRedo);
        if (m_undoToolbarAction) m_undoToolbarAction->setEnabled(canUndo);
        if (m_redoToolbarAction) m_redoToolbarAction->setEnabled(canRedo);
    });

    // Image list (thumbnail tabs) wiring
    if (m_imageListBar) {
        connect(m_imageListBar, &ImageListBar::imageSelected, this, &MainWindow::setActiveDocument);
        connect(m_imageListBar, &ImageListBar::imageCloseRequested, this, &MainWindow::closeDocument);
    }
    updateImageList();

    // Select brush tool by default
    selectTool(ToolType::Brush);
    // Place the four utility windows where paint.net puts them (Tools left,
    // History/Layers/Colors stacked right). resetUtilityWindow anchors them to
    // the app frame, so re-running it at every startup gives the same arrangement
    // whatever the window size is — coordinates saved by a differently sized
    // session would otherwise put them anywhere, even off-screen. Panels the user
    // docked or closed are left as they are.
    // Deferred so the docks are real top-level windows and the window manager has
    // finalised the main window's geometry (otherwise frameGeometry() is still
    // (0,0), the docks land outside the frame and move() is ignored).
    QTimer::singleShot(250, this, [this]() {
        for (QDockWidget *dock : {m_toolsDock, m_historyDock, m_layersDock, m_colorsDock}) {
            // window() != this, not isFloating(): a panel Qt has wrapped in a
            // QDockWidgetGroupWindow is its own window but reports isFloating()
            // false, and would keep whatever size the saved state gave it.
            if (dock && !dock->isHidden() && dock->window() != this)
                resetUtilityWindow(dock);
        }
        m_canvas->resetToDefaultView();   // re-centre once sizes are final
    });
    updateTitle();

    // Apply the current colour scheme (light / dark / follow-OS).
    applyTheme();

    // Keep the floating utility windows in sync with the app's minimise state
    // (some WMs don't send a WindowStateChange event on external minimise).
    auto *minimizeWatch = new QTimer(this);
    minimizeWatch->setInterval(200);
    connect(minimizeWatch, &QTimer::timeout, this, &MainWindow::syncUtilityWindowsToMainState);
    minimizeWatch->start();

    // Autosave every 60s so a crash doesn't lose everything.
    auto *autosaveTimer = new QTimer(this);
    autosaveTimer->setInterval(60 * 1000);
    connect(autosaveTimer, &QTimer::timeout, this, &MainWindow::performAutosave);
    autosaveTimer->start();

    // Offer to restore any autosaved image left behind by a previous crash.
    QTimer::singleShot(400, this, [this]() { checkForRecovery(); });
}

MainWindow::~MainWindow() {
}

void MainWindow::loadUiState() {
    QSettings settings("PaintDali", "PaintDali");

    // Geometry and state are stored as base64 strings to avoid Qt INI binary corruption.
    const QString geoB64 = settings.value("ui/mainWindowGeometry").toString();
    if (!geoB64.isEmpty()) {
        restoreGeometry(QByteArray::fromBase64(geoB64.toLatin1()));
    }

    // v3 layout persistence (bumped when the window layout was reworked to match
    // paint.net; any older saved state is intentionally ignored).
    const QString stateB64 = settings.value("ui/mainWindowStateV6").toString();
    if (!stateB64.isEmpty()) {
        const QByteArray savedState = QByteArray::fromBase64(stateB64.toLatin1());
        // Block normalizeDockLayout while restoreState repositions docks.
        QScopedValueRollback<bool> restoreGuard(m_restoringState, true);
        if (!restoreState(savedState, 6)) {
            settings.remove("ui/mainWindowStateV6");
            settings.remove("ui/mainWindowGeometry");
        }
    }

    auto restoreDock = [&settings](QDockWidget *dock, const char *key, bool defaultVisible) {
        if (!dock) return;
        dock->setVisible(settings.value(key, defaultVisible).toBool());
    };

    restoreDock(m_toolsDock, "ui/toolsVisible", true);
    restoreDock(m_historyDock, "ui/historyVisible", true);
    restoreDock(m_layersDock, "ui/layersVisible", true);
    restoreDock(m_colorsDock, "ui/colorsVisible", true);

    // restoreState() does NOT emit dockLocationChanged, so re-apply the tools
    // palette orientation to match the restored dock area (single thin row when
    // docked top/bottom, 2 columns otherwise).
    if (m_toolsDock && !m_toolsDock->isFloating()) {
        const Qt::DockWidgetArea area = dockWidgetArea(m_toolsDock);
        const bool horizontal = (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea);
        layoutToolsPalette(horizontal);
        if (horizontal)
            QTimer::singleShot(0, this, [this]() { resizeDocks({m_toolsDock}, {30}, Qt::Vertical); });
    }
}

void MainWindow::saveUiState() {
    QSettings settings("PaintDali", "PaintDali");

    // Store as base64 strings to avoid Qt INI format corrupting binary QByteArray data.
    settings.setValue("ui/mainWindowGeometry", QString(saveGeometry().toBase64()));
    settings.setValue("ui/mainWindowStateV6", QString(saveState(6).toBase64()));

    if (m_toolsDock) settings.setValue("ui/toolsVisible", m_toolsDock->isVisible());
    if (m_historyDock) settings.setValue("ui/historyVisible", m_historyDock->isVisible());
    if (m_layersDock) settings.setValue("ui/layersVisible", m_layersDock->isVisible());
    if (m_colorsDock) settings.setValue("ui/colorsVisible", m_colorsDock->isVisible());
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    Q_UNUSED(event);
    return QMainWindow::eventFilter(watched, event);
}

Qt::Orientation MainWindow::splitOrientationForArea(Qt::DockWidgetArea area) const {
    // Left/right areas keep narrow docks readable by stacking vertically.
    if (area == Qt::LeftDockWidgetArea || area == Qt::RightDockWidgetArea) {
        return Qt::Vertical;
    }
    // Top/bottom areas split horizontally so each panel keeps a practical height.
    return Qt::Horizontal;
}

void MainWindow::normalizeDockLayout(QDockWidget *changedDock) {
    if (!changedDock || changedDock->isFloating()) return;
    if (m_normalizingDockLayout) return;
    if (m_restoringState) return;

    QScopedValueRollback<bool> guard(m_normalizingDockLayout, true);

    Qt::DockWidgetArea area = dockWidgetArea(changedDock);
    if (area == Qt::NoDockWidgetArea) return;

    QList<QDockWidget*> allDocks = {m_toolsDock, m_historyDock, m_layersDock, m_colorsDock};
    QList<QDockWidget*> sameArea;
    for (QDockWidget *dock : allDocks) {
        if (!dock || dock->isFloating()) continue;
        if (dockWidgetArea(dock) == area) {
            sameArea.append(dock);
        }
    }

    if (sameArea.size() < 2) return;

    // Ensure minimum practical sizes so panels stay usable. The tools palette wants
    // a tall minimum only when it's a vertical column (left/right); docked top/bottom
    // it's a single short row, so a tall minimum would leave a big empty band.
    if (m_toolsDock) {
        const Qt::DockWidgetArea ta = dockWidgetArea(m_toolsDock);
        const bool toolsHorizontal = (ta == Qt::TopDockWidgetArea || ta == Qt::BottomDockWidgetArea);
        m_toolsDock->setMinimumSize(toolsHorizontal ? QSize(120, 30) : QSize(62, 240));
    }
    if (m_historyDock) m_historyDock->setMinimumSize(190, 130);
    if (m_layersDock) m_layersDock->setMinimumSize(190, 150);
    if (m_colorsDock) m_colorsDock->setMinimumSize(190, 190);

    const Qt::Orientation orientation = splitOrientationForArea(area);

    // If dock widgets overlap in the same area, split them to tile the area.
    for (QDockWidget *dock : sameArea) {
        if (dock == changedDock) continue;
        if (dock->geometry().intersects(changedDock->geometry())) {
            splitDockWidget(dock, changedDock, orientation);
            break;
        }
    }
}

void MainWindow::createTools() {
    m_tools.push_back(std::make_unique<SelectionTool>(SelectionShape::Rectangle));
    m_tools.push_back(std::make_unique<LassoTool>());
    m_tools.push_back(std::make_unique<SelectionTool>(SelectionShape::Ellipse));
    m_tools.push_back(std::make_unique<MagicWandTool>());
    m_tools.push_back(std::make_unique<MoveTool>());
    m_tools.push_back(std::make_unique<MoveSelectionTool>());
    m_tools.push_back(std::make_unique<ZoomTool>());
    m_tools.push_back(std::make_unique<PanTool>());
    m_tools.push_back(std::make_unique<BrushTool>());
    m_tools.push_back(std::make_unique<EraserTool>());
    m_tools.push_back(std::make_unique<PencilTool>());
    m_tools.push_back(std::make_unique<FillTool>());
    m_tools.push_back(std::make_unique<ColorPickerTool>());
    m_tools.push_back(std::make_unique<RecolorTool>());
    m_tools.push_back(std::make_unique<CloneStampTool>());
    m_tools.push_back(std::make_unique<TextTool>());
    m_tools.push_back(std::make_unique<LineTool>());
    m_tools.push_back(std::make_unique<ShapeTool>());
    m_tools.push_back(std::make_unique<GradientTool>());
}

void MainWindow::createMenus() {
    auto setShortcuts = [](QAction *action, std::initializer_list<QKeySequence> shortcuts) {
        action->setShortcuts(QList<QKeySequence>(shortcuts));
    };

    // Menu Fichier
    auto *fileMenu = menuBar()->addMenu(TR("&Fichier"));
    fileMenu->addAction(TR("&Nouveau..."), QKeySequence::New, this, &MainWindow::newDocument);
    fileMenu->addAction(TR("&Ouvrir..."), QKeySequence::Open, this, &MainWindow::openDocument);
    m_recentFilesMenu = fileMenu->addMenu(TR("Fichiers &récents"));
    m_recentFiles = QSettings().value("recentFiles").toStringList();
    updateRecentFilesMenu();
    fileMenu->addSeparator();
    fileMenu->addAction(TR("&Enregistrer"), QKeySequence::Save, this, &MainWindow::saveDocument);
    fileMenu->addAction(TR("Enregistrer &sous..."), QKeySequence("Ctrl+Shift+S"), this, &MainWindow::saveDocumentAs);
    fileMenu->addSeparator();
    fileMenu->addAction(TR("Im&primer..."), QKeySequence::Print, this, &MainWindow::printDocument);
    fileMenu->addSeparator();
    fileMenu->addAction(TR("&Fermer l'image"), QKeySequence("Ctrl+W"), this, &MainWindow::closeCurrentDocument);
    fileMenu->addAction(TR("&Quitter"), QKeySequence::Quit, this, &QMainWindow::close);

    // Menu Édition
    auto *editMenu = menuBar()->addMenu(TR("É&dition"));
    m_undoAction = editMenu->addAction(TR("&Annuler"), QKeySequence::Undo, this, &MainWindow::undo);
    m_redoAction = editMenu->addAction(TR("&Rétablir"), this, &MainWindow::redo);
    // paint.net redoes with Ctrl+Y; keep the platform default as an alias.
    setShortcuts(m_redoAction, {QKeySequence("Ctrl+Y"), QKeySequence::Redo});
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
    editMenu->addSeparator();
    auto *cutAction = editMenu->addAction(TR("C&ouper"), QKeySequence::Cut, this, &MainWindow::cut);
    auto *copyAction = editMenu->addAction(TR("&Copier"), QKeySequence::Copy, this, &MainWindow::copy);
    editMenu->addAction(TR("Copier &fusionné"), QKeySequence("Ctrl+Shift+C"), this, &MainWindow::copyMerged);
    auto *pasteAction = editMenu->addAction(TR("Co&ller"), QKeySequence::Paste, this, &MainWindow::paste);
    editMenu->addAction(TR("Coller dans un &nouveau calque"), QKeySequence("Ctrl+Shift+V"), this, &MainWindow::pasteIntoNewLayer);
    auto *deleteSelectionAction = editMenu->addAction(TR("&Supprimer"), QKeySequence(Qt::Key_Delete), this, &MainWindow::deleteSelectionContents);
    deleteSelectionAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    editMenu->addSeparator();
    auto *selectAllAction = editMenu->addAction(TR("&Tout sélectionner"), QKeySequence::SelectAll, this, &MainWindow::selectAll);
    auto *deselectAction = editMenu->addAction(TR("&Désélectionner"), QKeySequence("Ctrl+D"), this, &MainWindow::deselectAll);
    auto *invertSelectionAction = editMenu->addAction(TR("&Inverser la sélection"), QKeySequence("Ctrl+I"), this, &MainWindow::invertSelection);
    setShortcuts(cutAction, {QKeySequence::Cut, QKeySequence("Shift+Delete")});
    setShortcuts(copyAction, {QKeySequence::Copy, QKeySequence("Ctrl+Insert")});
    setShortcuts(pasteAction, {QKeySequence::Paste, QKeySequence("Shift+Insert")});
    setShortcuts(selectAllAction, {QKeySequence::SelectAll});
    setShortcuts(deselectAction, {QKeySequence("Ctrl+D")});
    setShortcuts(invertSelectionAction, {QKeySequence("Ctrl+I")});

    // Menu Affichage (3rd, as in paint.net: File, Edit, View, Image, Layers, Adjustments, Effects)
    auto *viewMenu = menuBar()->addMenu(TR("&Affichage"));
    viewMenu->addAction(TR("Zoom a&vant"), QKeySequence::ZoomIn, this, &MainWindow::zoomIn);
    viewMenu->addAction(TR("Zoom a&rrière"), QKeySequence::ZoomOut, this, &MainWindow::zoomOut);
    auto *zoomFitAction = viewMenu->addAction(TR("Zoom &adapté"), QKeySequence("Ctrl+B"), this, &MainWindow::zoomToFit);
    viewMenu->addAction(TR("Zoom sur la &sélection"), QKeySequence("Ctrl+Shift+B"), this, &MainWindow::zoomToSelection);
    auto *actualSizeAction = viewMenu->addAction(TR("Taille &réelle"), QKeySequence("Ctrl+0"), this, &MainWindow::zoomToActual);
    viewMenu->addSeparator();
    m_gridAction = viewMenu->addAction(TR("Afficher la &grille de pixels"), this, &MainWindow::toggleGrid);
    m_gridAction->setCheckable(true);
    m_rulersAction = viewMenu->addAction(TR("&Règles"), this, [this]() {
        bool show = !m_canvas->showRulers();
        m_canvas->setShowRulers(show);
        m_rulersAction->setChecked(show);
    });
    // Units submenu (Pixels / Inches / Centimeters).
    auto *unitsMenu = viewMenu->addMenu(TR("&Unités"));
    auto *unitGroup = new QActionGroup(this);
    auto addUnit = [&](const QString &label, CanvasWidget::Unit u, bool checked) {
        auto *a = unitsMenu->addAction(label, this, [this, u]() { m_canvas->setUnit(u); });
        a->setCheckable(true);
        a->setChecked(checked);
        unitGroup->addAction(a);
    };
    addUnit(TR("Pixels"), CanvasWidget::Unit::Pixels, true);
    addUnit(TR("Pouces"), CanvasWidget::Unit::Inches, false);
    addUnit(TR("Centimètres"), CanvasWidget::Unit::Centimeters, false);
    m_rulersAction->setCheckable(true);
    m_rulersAction->setChecked(true);
    setShortcuts(zoomFitAction, {QKeySequence("Ctrl+B")});
    setShortcuts(actualSizeAction, {QKeySequence("Ctrl+0"), QKeySequence("Ctrl+1")});

    // Menu Image
    auto *imageMenu = menuBar()->addMenu(TR("&Image"));
    imageMenu->addAction(TR("&Redimensionner..."), QKeySequence("Ctrl+R"), this, &MainWindow::resizeImage);
    imageMenu->addAction(TR("Taille du &canevas..."), QKeySequence("Ctrl+Shift+R"), this, &MainWindow::canvasSize);
    imageMenu->addSeparator();
    auto *rotateCwAction = imageMenu->addAction(TR("Rotation 90° horaire"), this, &MainWindow::rotateClockwise);
    auto *rotateCcwAction = imageMenu->addAction(TR("Rotation 90° anti-horaire"), this, &MainWindow::rotateCounterClockwise);
    imageMenu->addAction(TR("Rotation 180°"), this, &MainWindow::rotate180);
    imageMenu->addSeparator();
    imageMenu->addAction(TR("Retourner &horizontalement"), this, &MainWindow::flipHorizontal);
    imageMenu->addAction(TR("Retourner &verticalement"), this, &MainWindow::flipVertical);
    imageMenu->addSeparator();
    auto *cropAction = imageMenu->addAction(TR("Rogner selon la sélection"), this, &MainWindow::cropToSelection);
    auto *flattenAction = imageMenu->addAction(TR("&Aplatir"), this, &MainWindow::flattenImage);
    setShortcuts(rotateCwAction, {QKeySequence("Ctrl+H")});
    setShortcuts(rotateCcwAction, {QKeySequence("Ctrl+G")});
    setShortcuts(cropAction, {QKeySequence("Ctrl+Shift+X")});
    setShortcuts(flattenAction, {QKeySequence("Ctrl+Shift+F")});

    // Menu Calques
    auto *layersMenu = menuBar()->addMenu(TR("&Calques"));
    layersMenu->addAction(TR("Ajouter un calque"), QKeySequence("Ctrl+Shift+N"), this, [this]() { m_document->addLayer(); });
    auto *deleteLayerAction = layersMenu->addAction(TR("Supprimer le calque"), this, [this]() { m_document->removeLayer(m_document->activeLayerIndex()); });
    layersMenu->addAction(TR("Dupliquer le calque"), QKeySequence("Ctrl+Shift+D"), this, [this]() { m_document->duplicateLayer(m_document->activeLayerIndex()); });
    layersMenu->addAction(TR("Fusionner vers le bas"), QKeySequence("Ctrl+M"), this, [this]() { m_document->mergeLayerDown(m_document->activeLayerIndex()); });
    layersMenu->addSeparator();
    auto *layerPropertiesAction = layersMenu->addAction(TR("&Propriétés du calque..."), this, &MainWindow::showLayerPropertiesDialog);
    auto *toggleLayerVisibilityAction = layersMenu->addAction(TR("Basculer la visibilité"), this, &MainWindow::toggleActiveLayerVisibility);
    setShortcuts(deleteLayerAction, {QKeySequence("Ctrl+Shift+Delete")});
    setShortcuts(layerPropertiesAction, {QKeySequence("F4")});
    setShortcuts(toggleLayerVisibilityAction, {QKeySequence("Ctrl+,")});

    // Menu Ajustements
    auto *adjMenu = menuBar()->addMenu(TR("A&justements"));
    auto *autoLevelAction = adjMenu->addAction(TR("Niveau automatique"), this, [this]() { applyAdjustment(9); });
    adjMenu->addSeparator();
    auto *brightnessAction = adjMenu->addAction(TR("Luminosité / Contraste..."), this, [this]() { applyAdjustment(0); });
    auto *hueSatAction = adjMenu->addAction(TR("Teinte / Saturation..."), this, [this]() { applyAdjustment(1); });
    auto *levelsAction = adjMenu->addAction(TR("Niveaux..."), this, [this]() { applyAdjustment(2); });
    auto *curvesAction = adjMenu->addAction(TR("Courbes..."), this, [this]() { applyAdjustment(3); });
    adjMenu->addSeparator();
    auto *invertColorsAction = adjMenu->addAction(TR("Inverser les couleurs"), this, [this]() { applyAdjustment(4); });
    auto *sepiaAction = adjMenu->addAction(TR("Sépia..."), this, [this]() { applyAdjustment(5); });
    auto *posterizeAction = adjMenu->addAction(TR("Postériser..."), this, [this]() { applyAdjustment(6); });
    auto *blackWhiteAction = adjMenu->addAction(TR("Noir et blanc"), this, [this]() { applyAdjustment(7); });
    adjMenu->addAction(TR("Balance des couleurs..."), this, [this]() { applyAdjustment(8); });
    adjMenu->addSeparator();
    adjMenu->addAction(TR("Exposition..."), this, [this]() { applyAdjustment(10); });
    adjMenu->addAction(TR("Hautes / Basses lumières..."), this, [this]() { applyAdjustment(11); });
    adjMenu->addAction(TR("Température / Teinte..."), this, [this]() { applyAdjustment(12); });
    adjMenu->addAction(TR("Inverser l'alpha"), this, [this]() { applyAdjustment(13); });
    setShortcuts(autoLevelAction, {QKeySequence("Ctrl+Shift+L")});
    setShortcuts(brightnessAction, {QKeySequence("Ctrl+Shift+T")});
    setShortcuts(hueSatAction, {QKeySequence("Ctrl+Shift+U")});
    setShortcuts(levelsAction, {QKeySequence("Ctrl+L")});
    setShortcuts(curvesAction, {QKeySequence("Ctrl+Shift+M")});
    setShortcuts(invertColorsAction, {QKeySequence("Ctrl+Shift+I")});
    setShortcuts(sepiaAction, {QKeySequence("Ctrl+Shift+E")});
    setShortcuts(posterizeAction, {QKeySequence("Ctrl+Shift+P")});
    setShortcuts(blackWhiteAction, {QKeySequence("Ctrl+Shift+G")});

    // Menu Effets
    auto *fxMenu = menuBar()->addMenu(TR("E&ffets"));

    m_repeatEffectAction = fxMenu->addAction(TR("Répéter le dernier effet"), this, [this]() {
        if (m_lastEffect) m_lastEffect();
    });
    m_repeatEffectAction->setShortcut(QKeySequence("Ctrl+F"));
    m_repeatEffectAction->setEnabled(false);
    fxMenu->addSeparator();

    auto *artisticMenu = fxMenu->addMenu(TR("Artistique"));
    artisticMenu->addAction(TR("Croquis à l'encre..."), this, [this]() { applyEffect(8); });
    artisticMenu->addAction(TR("Peinture à l'huile..."), this, [this]() { applyEffect(5); });
    artisticMenu->addAction(TR("Croquis au crayon..."), this, [this]() { applyEffect(9); });

    auto *blursMenu = fxMenu->addMenu(TR("Flous"));
    blursMenu->addAction(TR("Fragment..."), this, [this]() { applyEffect(17); });
    blursMenu->addAction(TR("Flou gaussien..."), this, [this]() { applyEffect(0); });
    blursMenu->addAction(TR("Flou directionnel..."), this, [this]() { applyEffect(7); });
    blursMenu->addAction(TR("Flou radial..."), this, [this]() { applyEffect(13); });
    blursMenu->addAction(TR("Flou de surface..."), this, [this]() { applyEffect(15); });
    blursMenu->addAction(TR("Flou..."), this, [this]() { applyEffect(16); });
    blursMenu->addAction(TR("Flou de zoom..."), this, [this]() { applyEffect(14); });

    auto *distortMenu = fxMenu->addMenu(TR("Déformation"));
    distortMenu->addAction(TR("Bombement..."), this, [this]() { applyEffect(18); });
    distortMenu->addAction(TR("Cristalliser..."), this, [this]() { applyEffect(20); });
    distortMenu->addAction(TR("Bosselure..."), this, [this]() { applyEffect(22); });
    distortMenu->addAction(TR("Verre givré..."), this, [this]() { applyEffect(19); });
    distortMenu->addAction(TR("Pixéliser..."), this, [this]() { applyEffect(6); });
    distortMenu->addAction(TR("Inversion polaire..."), this, [this]() { applyEffect(23); });
    distortMenu->addAction(TR("Réflexion en mosaïque..."), this, [this]() { applyEffect(21); });
    distortMenu->addAction(TR("Torsion..."), this, [this]() { applyEffect(24); });

    auto *noiseMenu = fxMenu->addMenu(TR("Bruit"));
    noiseMenu->addAction(TR("Ajouter du bruit..."), this, [this]() { applyEffect(2); });
    noiseMenu->addAction(TR("Médiane..."), this, [this]() { applyEffect(25); });
    noiseMenu->addAction(TR("Quantifier..."), this, [this]() { applyEffect(27); });
    noiseMenu->addAction(TR("Réduire le bruit..."), this, [this]() { applyEffect(26); });

    auto *objectMenu = fxMenu->addMenu(TR("Objet"));
    objectMenu->addAction(TR("Ombre portée..."), this, [this]() { applyEffect(36); });

    auto *photoMenu = fxMenu->addMenu(TR("Photo"));
    photoMenu->addAction(TR("Lueur..."), this, [this]() { applyEffect(28); });
    photoMenu->addAction(TR("Suppression yeux rouges..."), this, [this]() { applyEffect(29); });
    photoMenu->addAction(TR("Netteté..."), this, [this]() { applyEffect(1); });
    photoMenu->addAction(TR("Adoucir le portrait..."), this, [this]() { applyEffect(30); });
    photoMenu->addAction(TR("Vignette..."), this, [this]() { applyEffect(31); });

    auto *renderMenu = fxMenu->addMenu(TR("Rendu"));
    renderMenu->addAction(TR("Nuages..."), this, [this]() { applyEffect(10); });
    renderMenu->addAction(TR("Fractale Julia..."), this, [this]() { applyEffect(11); });
    renderMenu->addAction(TR("Fractale Mandelbrot..."), this, [this]() { applyEffect(12); });
    renderMenu->addAction(TR("Turbulence..."), this, [this]() { applyEffect(32); });

    auto *stylizeMenu = fxMenu->addMenu(TR("Styliser"));
    stylizeMenu->addAction(TR("Détection de contours"), this, [this]() { applyEffect(4); });
    stylizeMenu->addAction(TR("Estampage..."), this, [this]() { applyEffect(3); });
    stylizeMenu->addAction(TR("Morphologie..."), this, [this]() { applyEffect(35); });
    stylizeMenu->addAction(TR("Contour..."), this, [this]() { applyEffect(34); });
    stylizeMenu->addAction(TR("Relief..."), this, [this]() { applyEffect(33); });

    // External effect plugins (Effets ▸ Plugins). Entirely additive — the submenu is
    // always present so the feature is discoverable, and lists whatever loaded.
    fxMenu->addSeparator();
    auto *pluginsMenu = fxMenu->addMenu(TR("Plugins"));
    populatePluginsMenu(pluginsMenu);

    // paint.net has no Window/Help menus: the four utility windows plus Settings
    // and Help live as six icons on the RIGHT side of the menu bar.
    createMenuBarCornerIcons();
}

void MainWindow::populatePluginsMenu(QMenu *pluginsMenu) {
    if (!pluginsMenu) return;

    if (!m_pluginManager) {
        m_pluginManager = std::make_unique<PluginManager>();
        m_pluginManager->loadFrom(PluginManager::defaultPluginDirs());
    }

    const auto &effects = m_pluginManager->effects();
    if (effects.empty()) {
        QAction *none = pluginsMenu->addAction(TR("Aucun plugin chargé"));
        none->setEnabled(false);
    } else {
        for (const auto &eff : effects) {
            PluginEffect *raw = eff.get();
            pluginsMenu->addAction(eff->name(), this, [this, raw]() { runPluginEffect(raw); });
        }
    }

    pluginsMenu->addSeparator();
    pluginsMenu->addAction(TR("Ouvrir le dossier des plugins…"), this, [this]() {
        const QStringList dirs = PluginManager::defaultPluginDirs();
        if (dirs.isEmpty()) return;
        const QString target = dirs.last();   // the writable app-data plugins dir
        QDir().mkpath(target);
        QDesktopServices::openUrl(QUrl::fromLocalFile(target));
    });
}

void MainWindow::runPluginEffect(PluginEffect *effect) {
    if (!effect || !m_document) return;
    auto *layer = m_document->activeLayer();
    if (!layer) return;

    if (effect->paramCount() == 0) {
        applyImageOperationToTargetLayers(
            [effect](const QImage &image) { return effect->apply(image); }, effect->name());
        return;
    }

    QVector<PreviewDialog::Param> dlgParams;
    for (const auto &p : effect->params())
        dlgParams.push_back({p.label, p.minValue, p.maxValue, p.defValue, p.suffix});

    PreviewDialog dlg(effect->name(), layer->image(), dlgParams,
        [effect](const QImage &src, const QVector<int> &v) {
            effect->setValues(v);
            return effect->apply(src);
        }, this);
    if (dlg.exec() != QDialog::Accepted) return;

    effect->setValues(dlg.values());
    applyImageOperationToTargetLayers(
        [effect](const QImage &image) { return effect->apply(image); }, effect->name());
}

void MainWindow::rebuildToolsPaletteTooltips() {
    for (QToolButton *btn : m_toolButtons) {
        if (!btn) continue;
        QString tip = "<b>" + TR(btn->property("toolName").toString()) + "</b>";
        const QString key = btn->property("toolKey").toString();
        if (!key.isEmpty()) tip += " (" + key + ")";
        const QString desc = toolDescription(static_cast<ToolType>(btn->property("toolTypeInt").toInt()));
        if (!desc.isEmpty()) tip += "<br>" + TR(desc);
        btn->setToolTip(tip);
    }
}

void MainWindow::showShortcutsHelp() {
    const QString html = QString(
        "<h3>%1</h3>"
        "<table cellspacing='6'>"
        "<tr><td><b>%2</b></td><td>S · S×2 · S×3 · S×4</td></tr>"
        "<tr><td><b>%3</b></td><td>M · M×2</td></tr>"
        "<tr><td><b>%4</b></td><td>O · O×2</td></tr>"
        "<tr><td>%5</td><td>B · P · E · F · G · H · K · L · R · T · Z</td></tr>"
        "<tr><td>%6</td><td>[ ] &nbsp; Ctrl+[ Ctrl+]</td></tr>"
        "<tr><td>%7</td><td>X · C</td></tr>"
        "<tr><td>%8</td><td>F5 · F6 · F7 · F8</td></tr>"
        "<tr><td>%9</td><td>Ctrl+Maj+F5…F8</td></tr>"
        "<tr><td>%10</td><td>Ctrl+Z · Ctrl+Y</td></tr>"
        "<tr><td>%11</td><td>Ctrl+F</td></tr>"
        "</table>")
        .arg(TR("Raccourcis clavier"),
             TR("Sélections (cycle)"),
             TR("Déplacement (cycle)"),
             TR("Ligne / Formes (cycle)"),
             TR("Outils directs"),
             TR("Taille du pinceau"),
             TR("Échanger / basculer les couleurs"),
             TR("Fenêtres Outils / Historique / Calques / Couleurs"),
             TR("Replacer une fenêtre"))
        .arg(TR("Annuler / Rétablir"),
             TR("Répéter le dernier effet"));

    QMessageBox box(this);
    box.setWindowTitle(TR("Raccourcis clavier"));
    box.setTextFormat(Qt::RichText);
    box.setText(html);
    box.exec();
}

void MainWindow::applyTheme() {
    setStyleSheet(Theme::styleSheet());
    if (m_canvas) {
        m_canvas->setBackdropColor(QColor(Theme::canvasBackdrop()));
        m_canvas->update();
    }
    // Floating utility windows are separate top-level widgets; restyle them too.
    for (QDockWidget *dock : {m_toolsDock, m_historyDock, m_layersDock, m_colorsDock}) {
        if (dock) dock->setStyleSheet(Theme::styleSheet());
    }
    // The reset/swap icons are painted pixmaps, so a stylesheet can't recolour
    // them: redraw them for the new scheme.
    if (m_colorsPanel) m_colorsPanel->refreshIcons();
}

void MainWindow::retranslateUi() {
    // Rebuild every piece of chrome that carries text, in the new language.
    if (QWidget *old = menuBar()->cornerWidget(Qt::TopRightCorner)) {
        menuBar()->setCornerWidget(nullptr, Qt::TopRightCorner);
        old->deleteLater();
    }
    menuBar()->clear();
    m_recentFilesMenu = nullptr;
    m_repeatEffectAction = nullptr;
    createMenus();          // also re-creates the corner icons

    if (m_fixedToolbar) {
        m_fixedToolbar->clear();
        populateFixedToolbar();
    }
    if (m_toolOptionsPanel) m_toolOptionsPanel->retranslate();
    if (m_layersPanel) m_layersPanel->retranslate();

    if (m_toolsDock)   m_toolsDock->setWindowTitle(TR("Outils"));
    if (m_historyDock) m_historyDock->setWindowTitle(TR("Historique"));
    if (m_layersDock)  m_layersDock->setWindowTitle(TR("Calques"));
    if (m_colorsDock)  m_colorsDock->setWindowTitle(TR("Couleurs"));

    rebuildToolsPaletteTooltips();

    // Re-running tool selection refreshes the status-bar name and help text.
    if (m_currentTool) selectTool(m_currentTool->type());

    // Undo/redo enablement was lost with the old actions.
    if (m_document) {
        const bool canUndo = m_document->history().canUndo();
        const bool canRedo = m_document->history().canRedo();
        if (m_undoAction) m_undoAction->setEnabled(canUndo);
        if (m_redoAction) m_redoAction->setEnabled(canRedo);
        if (m_undoToolbarAction) m_undoToolbarAction->setEnabled(canUndo);
        if (m_redoToolbarAction) m_redoToolbarAction->setEnabled(canRedo);
    }
    updateTitle();
}

void MainWindow::showSettingsDialog() {
    SettingsDialog dlg(this);
    connect(&dlg, &SettingsDialog::colorSchemeChanged, this, [this, &dlg]() {
        applyTheme();
        dlg.setStyleSheet(Theme::styleSheet());
    });
    connect(&dlg, &SettingsDialog::languageChanged, this, [this]() { retranslateUi(); });
    connect(&dlg, &SettingsDialog::canvasSettingsChanged, this, [this]() {
        if (m_canvas) m_canvas->reloadSettings();
    });
    dlg.exec();
}

void MainWindow::createMenuBarCornerIcons() {
    auto *corner = new QWidget(this);
    auto *cornerLayout = new QHBoxLayout(corner);
    cornerLayout->setContentsMargins(0, 0, 6, 0);
    cornerLayout->setSpacing(2);

    // Helper: a checkable toggle button bound to a dock, with an F-key shortcut.
    // The tooltip spells out what the window is for and how to reset it.
    auto addDockToggle = [&](QDockWidget *dock, const QIcon &icon, const QString &label,
                             const QString &key, const QString &what) {
        auto *btn = new QToolButton(corner);
        btn->setIcon(icon);
        btn->setIconSize(QSize(16, 16));
        btn->setCheckable(true);
        btn->setChecked(dock->isVisible());
        btn->setToolTip(QString("<b>%1</b> (%2)<br>%3<br><i>%4</i>")
                            .arg(TR(label), key, TR(what),
                                 TR("Ctrl+Maj+" ) + key + " : " + TR("replacer la fenêtre")));
        btn->setAutoRaise(true);
        // "Active" state = a blue underline, NOT a filled blue background: the global
        // QToolButton:checked fill hid the blue Tools/History icons (blue on blue) in
        // dark mode. An underline keeps every icon fully visible in both themes.
        btn->setStyleSheet(
            "QToolButton { background: transparent; border: none;"
            " border-bottom: 2px solid transparent; border-radius: 3px; padding: 2px 2px 0 2px; }"
            "QToolButton:hover { background: rgba(127,127,127,0.28); }"
            "QToolButton:checked { background: transparent; border-bottom: 2px solid #4a90d9; }");
        connect(btn, &QToolButton::clicked, this, [dock](bool on) { dock->setVisible(on); });
        connect(dock, &QDockWidget::visibilityChanged, btn, &QToolButton::setChecked);

        // F5-F8 toggle; Ctrl+Shift+F* resets position (like paint.net).
        auto *toggle = new QAction(TR(label), this);
        toggle->setShortcut(QKeySequence(key));
        connect(toggle, &QAction::triggered, this, [dock]() { dock->setVisible(!dock->isVisible()); });
        addAction(toggle);

        auto *reset = new QAction("Reset " + label, this);
        reset->setShortcut(QKeySequence("Ctrl+Shift+" + key));
        connect(reset, &QAction::triggered, this, [this, dock]() { resetUtilityWindow(dock); });
        addAction(reset);

        cornerLayout->addWidget(btn);
        return btn;
    };

    addDockToggle(m_toolsDock, ToolIcons::toolsWindow(), "Outils", "F5",
                  "Affiche ou masque la palette d'outils (pinceau, sélection, formes...).");
    addDockToggle(m_historyDock, ToolIcons::historyWindow(), "Historique", "F6",
                  "Affiche ou masque l'historique : chaque action est listée, cliquez pour y revenir.");
    addDockToggle(m_layersDock, ToolIcons::layersWindow(), "Calques", "F7",
                  "Affiche ou masque les calques : ajouter, supprimer, réordonner, opacité et fusion.");
    addDockToggle(m_colorsDock, ToolIcons::colorsWindow(), "Couleurs", "F8",
                  "Affiche ou masque les couleurs : couleur primaire/secondaire, roue, RVB/TSV et palette.");

    // Settings
    auto *settingsBtn = new QToolButton(corner);
    settingsBtn->setIcon(ToolIcons::settings());
    settingsBtn->setIconSize(QSize(16, 16));
    settingsBtn->setAutoRaise(true);
    settingsBtn->setToolTip(QString("<b>%1</b><br>%2")
        .arg(TR("Paramètres"),
             TR("Ouvre les paramètres : langue (français / anglais), thème clair ou sombre, et canevas.")));
    connect(settingsBtn, &QToolButton::clicked, this, &MainWindow::showSettingsDialog);
    cornerLayout->addWidget(settingsBtn);

    // Help
    auto *helpBtn = new QToolButton(corner);
    helpBtn->setIcon(ToolIcons::help());
    helpBtn->setIconSize(QSize(16, 16));
    helpBtn->setAutoRaise(true);
    helpBtn->setToolTip(QString("<b>%1</b><br>%2")
        .arg(TR("Aide"),
             TR("Raccourcis clavier, documentation et informations sur la version.")));
    helpBtn->setPopupMode(QToolButton::InstantPopup);
    auto *helpMenu = new QMenu(helpBtn);
    helpMenu->addAction(TR("Raccourcis clavier"), this, &MainWindow::showShortcutsHelp);
    helpMenu->addSeparator();
    helpMenu->addAction(TR("À &propos de paint.software"), this, [this]() {
        QMessageBox::about(this, TR("À propos de paint.software"),
            "<h2>paint.software</h2>"
            "<p>Version 1.0 — édition Linux</p>"
            "<p>Un éditeur d'images puissant construit avec Qt et C++.</p>");
    });
    helpBtn->setMenu(helpMenu);
    cornerLayout->addWidget(helpBtn);

    menuBar()->setCornerWidget(corner, Qt::TopRightCorner);
}

void MainWindow::resetUtilityWindow(QDockWidget *dock) {
    // Restore a utility window to its default floating position, anchored to the
    // canvas area: Tools hug the left edge, History/Layers/Colors stack down the
    // right edge — the paint.net default arrangement.
    if (!dock || !m_canvas) return;
    dock->setVisible(true);
    // Only float it if it is currently docked into the main window. When Qt has
    // wrapped it in a QDockWidgetGroupWindow it is already its own window, and
    // calling setFloating() would tear it out — leaving the emptied group window
    // behind as a blank top-level window. Position the wrapper instead.
    if (dock->window() == this) dock->setFloating(true);
    QWidget *top = dock->window();
    if (!top || top == this) return;

    // Default arrangement, reproducing the user's preferred layout. Left/top
    // windows anchor to the app's top-left, right/bottom windows to the app's
    // right/bottom edge, so it holds up if the window is resized. Everything is
    // clamped to stay fully inside the app frame.
    //   Tools  -> upper-left (over the canvas)   History -> upper-right
    //   Colors -> lower-left                      Layers  -> lower-right
    const QRect app = frameGeometry();          // includes the WM title bar
    const int rightW = 262;

    auto place = [&](int x, int y, int w, int h) {
        x = std::max(app.left() + 6, std::min(x, app.right() - w - 6));
        y = std::max(app.top() + 6, std::min(y, app.bottom() - h - 6));
        top->resize(w, h);
        top->move(x, y);
    };

    if (dock == m_toolsDock) {
        place(app.left() + 65, app.top() + 164, top->sizeHint().width(), 312);
    } else if (dock == m_historyDock) {
        place(app.right() - rightW - 28, app.top() + 153, rightW, 300);
    } else if (dock == m_layersDock) {
        place(app.right() - rightW - 28, app.bottom() - 260 - 78, rightW, 260);
    } else if (dock == m_colorsDock) {
        // Opens collapsed (wheel + palette); "Plus >>" adds the sliders. Take the
        // height from the panel: hard-coding it either crops the palette or, since
        // Qt enforces the minimum anyway, anchors the window too low.
        const int h = std::max(300, top->sizeHint().height());
        place(app.left() + 28, app.bottom() - h - 76, 230, h);
    }
}

QAction *MainWindow::addShortcutAction(const QString &text, const QList<QKeySequence> &shortcuts,
                                       const std::function<void()> &handler,
                                       Qt::ShortcutContext context) {
    auto *action = new QAction(text, this);
    action->setShortcuts(shortcuts);
    action->setShortcutContext(context);
    connect(action, &QAction::triggered, this, [handler]() { handler(); });
    addAction(action);
    return action;
}

bool MainWindow::allowSingleKeyShortcuts() const {
    QWidget *focus = QApplication::focusWidget();
    if (!focus) return true;
    if (focus == m_canvas && m_currentTool && m_currentTool->type() == ToolType::Text) return false;
    return !focus->inherits("QLineEdit")
        && !focus->inherits("QTextEdit")
        && !focus->inherits("QPlainTextEdit")
        && !focus->inherits("QAbstractSpinBox")
        && !focus->inherits("QComboBox");
}

void MainWindow::cycleToolGroup(const std::vector<ToolType> &types, bool reverse) {
    if (!allowSingleKeyShortcuts() || types.empty()) return;

    int currentIndex = -1;
    for (size_t i = 0; i < types.size(); ++i) {
        if (m_currentTool && m_currentTool->type() == types[i]) {
            currentIndex = static_cast<int>(i);
            break;
        }
    }

    int nextIndex = 0;
    if (currentIndex >= 0) {
        int delta = reverse ? -1 : 1;
        nextIndex = (currentIndex + delta + static_cast<int>(types.size())) % static_cast<int>(types.size());
    } else if (reverse) {
        nextIndex = static_cast<int>(types.size()) - 1;
    }

    selectTool(types[nextIndex]);
}

void MainWindow::selectLayerAbove() {
    if (!m_document) return;
    int nextIndex = std::min(m_document->activeLayerIndex() + 1, m_document->layerCount() - 1);
    m_document->setActiveLayer(nextIndex);
}

void MainWindow::selectLayerBelow() {
    if (!m_document) return;
    int nextIndex = std::max(m_document->activeLayerIndex() - 1, 0);
    m_document->setActiveLayer(nextIndex);
}

void MainWindow::selectTopLayer() {
    if (!m_document || m_document->layerCount() == 0) return;
    m_document->setActiveLayer(m_document->layerCount() - 1);
}

void MainWindow::selectBottomLayer() {
    if (!m_document || m_document->layerCount() == 0) return;
    m_document->setActiveLayer(0);
}

void MainWindow::adjustCurrentToolBrushSize(int delta) {
    if (!m_currentTool || !allowSingleKeyShortcuts()) return;
    m_currentTool->setBrushSize(m_currentTool->brushSize() + delta);
    if (m_toolOptionsPanel) {
        m_toolOptionsPanel->setTool(m_currentTool);
    }
}

void MainWindow::toggleActiveLayerVisibility() {
    auto *layer = m_document ? m_document->activeLayer() : nullptr;
    if (!layer) return;
    layer->setVisible(!layer->isVisible());
    emit m_document->layersChanged();
    emit m_document->documentChanged();
}

void MainWindow::showLayerPropertiesDialog() {
    auto *layer = m_document ? m_document->activeLayer() : nullptr;
    if (!layer) return;

    // Remember the original state so Cancel can restore it after live previews.
    const float origOpacity = layer->opacity();
    const bool origVisible = layer->isVisible();
    const BlendMode origBlend = layer->blendMode();
    const auto modes = Layer::allBlendModes();

    LayerPropertiesDialog dlg(layer, this);
    // Apply every change to the canvas immediately for a live preview.
    connect(&dlg, &LayerPropertiesDialog::previewChanged, this,
            [this, layer, &modes](int opacity255, bool visible, int blendIdx) {
        layer->setOpacity(opacity255 / 255.0f);
        layer->setVisible(visible);
        if (blendIdx >= 0 && blendIdx < modes.size())
            layer->setBlendMode(modes[blendIdx]);
        emit m_document->documentChanged();
        emit m_document->layersChanged();
    });

    if (dlg.exec() == QDialog::Accepted) {
        // Restore the original state, then apply the final one THROUGH history so
        // the whole change is a single undoable step.
        layer->setOpacity(origOpacity);
        layer->setVisible(origVisible);
        layer->setBlendMode(origBlend);
        const int idx = m_document->activeLayerIndex();
        auto cmd = std::make_unique<LayerPropertyCommand>(
            m_document, idx, origOpacity, origVisible, static_cast<int>(origBlend),
            layer->name(), layer->isLocked(), "Propriétés du calque");
        layer->setName(dlg.layerName());
        layer->setVisible(dlg.isVisible());
        layer->setBlendMode(dlg.blendMode());
        layer->setOpacity(dlg.opacity() / 255.0f);
        cmd->captureAfter();
        m_document->history().push(std::move(cmd));
        emit m_document->layersChanged();
        emit m_document->documentChanged();
    } else {
        // Cancelled: revert to the original state.
        layer->setOpacity(origOpacity);
        layer->setVisible(origVisible);
        layer->setBlendMode(origBlend);
        emit m_document->documentChanged();
        emit m_document->layersChanged();
    }
}

void MainWindow::createKeyboardShortcuts() {
    addShortcutAction("Desélectionner", {QKeySequence("Return"), QKeySequence("Enter")}, [this]() {
        if (!m_document || !m_document->selection().hasSelection()) return;
        if (m_currentTool && m_currentTool->type() == ToolType::Text) return;
        deselectAll();
    });

    addShortcutAction("Selection tools", {QKeySequence("S")}, [this]() {
        cycleToolGroup({ToolType::RectSelection, ToolType::LassoSelection, ToolType::EllipseSelection, ToolType::MagicWand});
    });
    addShortcutAction("Selection tools reverse", {QKeySequence("Shift+S")}, [this]() {
        cycleToolGroup({ToolType::RectSelection, ToolType::LassoSelection, ToolType::EllipseSelection, ToolType::MagicWand}, true);
    });
    addShortcutAction("Move tools", {QKeySequence("M")}, [this]() {
        cycleToolGroup({ToolType::Move, ToolType::MoveSelection});
    });
    addShortcutAction("Move tools reverse", {QKeySequence("Shift+M")}, [this]() {
        cycleToolGroup({ToolType::Move, ToolType::MoveSelection}, true);
    });
    addShortcutAction("Shape tools", {QKeySequence("O")}, [this]() {
        cycleToolGroup({ToolType::Line, ToolType::Shape});
    });
    addShortcutAction("Shape tools reverse", {QKeySequence("Shift+O")}, [this]() {
        cycleToolGroup({ToolType::Line, ToolType::Shape}, true);
    });

    auto addToolShortcut = [this](const QString &text, const QString &shortcut, ToolType type) {
        addShortcutAction(text, {QKeySequence(shortcut)}, [this, type]() {
            if (!allowSingleKeyShortcuts()) return;
            selectTool(type);
        });
    };

    addToolShortcut("Brush tool", "B", ToolType::Brush);
    addToolShortcut("Pencil tool", "P", ToolType::Pencil);
    addToolShortcut("Eraser tool", "E", ToolType::Eraser);
    addToolShortcut("Fill tool", "F", ToolType::Fill);
    addToolShortcut("Gradient tool", "G", ToolType::Gradient);
    addToolShortcut("Pan tool", "H", ToolType::Pan);
    addToolShortcut("Color picker tool", "K", ToolType::ColorPicker);
    addToolShortcut("Clone stamp tool", "L", ToolType::CloneStamp);
    addToolShortcut("Recolor tool", "R", ToolType::Recolor);
    addToolShortcut("Text tool", "T", ToolType::Text);
    addToolShortcut("Zoom tool", "Z", ToolType::Zoom);
    // NB: no W / U bindings — in paint.net the Magic Wand is reached by cycling S
    // and Shapes by cycling O (handled by the cycleToolGroup shortcuts above).

    addShortcutAction("Swap colors", {QKeySequence("X")}, [this]() {
        if (!allowSingleKeyShortcuts() || !m_colorsPanel) return;
        m_colorsPanel->swapPrimaryAndSecondaryColors();
    });
    addShortcutAction("Toggle active color", {QKeySequence("C")}, [this]() {
        if (!allowSingleKeyShortcuts() || !m_colorsPanel) return;
        m_colorsPanel->toggleActiveColorSlot();
    });

    addShortcutAction("Layer above", {QKeySequence("Alt+PgUp")}, [this]() { selectLayerAbove(); });
    addShortcutAction("Layer below", {QKeySequence("Alt+PgDown")}, [this]() { selectLayerBelow(); });
    addShortcutAction("Top layer", {QKeySequence("Ctrl+Alt+PgUp")}, [this]() { selectTopLayer(); });
    addShortcutAction("Bottom layer", {QKeySequence("Ctrl+Alt+PgDown")}, [this]() { selectBottomLayer(); });
    addShortcutAction("Brush size down", {QKeySequence("[")}, [this]() { adjustCurrentToolBrushSize(-1); });
    addShortcutAction("Brush size up", {QKeySequence("]")}, [this]() { adjustCurrentToolBrushSize(1); });
    addShortcutAction("Brush size down fast", {QKeySequence("Ctrl+[")}, [this]() { adjustCurrentToolBrushSize(-5); });
    addShortcutAction("Brush size up fast", {QKeySequence("Ctrl+]")}, [this]() { adjustCurrentToolBrushSize(5); });
}

void MainWindow::createToolBar() {
    auto *toolbar = addToolBar(TR("Barre d'outils"));
    toolbar->setObjectName("FixedToolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));
    m_fixedToolbar = toolbar;
    populateFixedToolbar();
}

void MainWindow::populateFixedToolbar() {
    QToolBar *toolbar = m_fixedToolbar;
    if (!toolbar) return;

    // Exact paint.net toolbar order:
    // New, Open, Save, Print | Cut, Copy, Paste | Crop, Deselect | Undo, Redo | Grid, Rulers
    toolbar->addAction(ToolIcons::newDoc(), TR("Nouveau"), this, &MainWindow::newDocument);
    toolbar->addAction(ToolIcons::openDoc(), TR("Ouvrir"), this, &MainWindow::openDocument);
    toolbar->addAction(ToolIcons::saveDoc(), TR("Enregistrer"), this, &MainWindow::saveDocument);
    toolbar->addAction(ToolIcons::printAction(), TR("Imprimer"), this, &MainWindow::printDocument);
    toolbar->addSeparator();
    toolbar->addAction(ToolIcons::cutAction(), TR("Couper"), this, &MainWindow::cut);
    toolbar->addAction(ToolIcons::copyAction(), TR("Copier"), this, &MainWindow::copy);
    toolbar->addAction(ToolIcons::pasteAction(), TR("Coller"), this, &MainWindow::paste);
    toolbar->addSeparator();
    toolbar->addAction(ToolIcons::cropAction(), TR("Rogner selon la sélection"), this, &MainWindow::cropToSelection);
    toolbar->addAction(ToolIcons::deselectAction(), TR("Désélectionner"), this, &MainWindow::deselectAll);
    toolbar->addSeparator();
    m_undoToolbarAction = toolbar->addAction(ToolIcons::undoAction(), TR("Annuler"), this, &MainWindow::undo);
    m_redoToolbarAction = toolbar->addAction(ToolIcons::redoAction(), TR("Rétablir"), this, &MainWindow::redo);
    m_undoToolbarAction->setEnabled(false);
    m_redoToolbarAction->setEnabled(false);
    toolbar->addSeparator();

    auto *gridToggle = toolbar->addAction(ToolIcons::pixelGridAction(), TR("Grille de pixels"), this, &MainWindow::toggleGrid);
    gridToggle->setCheckable(true);
    gridToggle->setChecked(m_canvas && m_canvas->showGrid());
    auto *rulersToggle = toolbar->addAction(ToolIcons::rulersAction(), TR("Règles"), this, [this]() {
        bool show = !m_canvas->showRulers();
        m_canvas->setShowRulers(show);
        if (m_rulersAction) m_rulersAction->setChecked(show);
    });
    rulersToggle->setCheckable(true);
    rulersToggle->setChecked(m_canvas ? m_canvas->showRulers() : true);

    // Keep the toolbar toggles and the View-menu entries in sync.
    if (m_gridAction) {
        connect(m_gridAction, &QAction::toggled, gridToggle, &QAction::setChecked);
        connect(gridToggle, &QAction::toggled, m_gridAction, &QAction::setChecked);
    }
    if (m_rulersAction)
        connect(m_rulersAction, &QAction::toggled, rulersToggle, &QAction::setChecked);

    // Right side of the toolbar: the open-image thumbnail list (like browser tabs).
    auto *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // QToolBar::clear() destroys the previous bar on retranslate, so recreate it
    // and re-establish its connections here.
    m_imageListBar = new ImageListBar(this);
    connect(m_imageListBar, &ImageListBar::imageSelected, this, &MainWindow::setActiveDocument);
    connect(m_imageListBar, &ImageListBar::imageCloseRequested, this, &MainWindow::closeDocument);
    toolbar->addWidget(m_imageListBar);
    updateImageList();

    // Force top bars to match Paint.NET structure:
    // row 1: menu, row 2: fixed toolbar, row 3: variable toolbar.
    if (m_variableToolbar) {
        insertToolBar(m_variableToolbar, m_fixedToolbar);
        insertToolBarBreak(m_variableToolbar);
    }
}

// One-line explanation of what each tool does, shown in its tooltip.
// (Forward-declared near the top so rebuildToolsPaletteTooltips can use it.)
QString toolDescription(ToolType t) {
    switch (t) {
    case ToolType::RectSelection:   return "Sélectionne une zone rectangulaire à modifier.";
    case ToolType::EllipseSelection:return "Sélectionne une zone ovale à modifier.";
    case ToolType::LassoSelection:  return "Sélectionne une zone à main levée en la traçant.";
    case ToolType::MagicWand:       return "Sélectionne automatiquement une plage de couleurs similaires.";
    case ToolType::Move:            return "Déplace les pixels de la sélection (ou tout le calque).";
    case ToolType::MoveSelection:   return "Déplace le contour de la sélection sans déplacer les pixels.";
    case ToolType::Zoom:            return "Agrandit (clic gauche) ou réduit (clic droit) l'affichage.";
    case ToolType::Pan:             return "Déplace la vue dans l'image en cliquant-glissant (main).";
    case ToolType::Fill:            return "Remplit une zone de couleur uniforme avec la couleur active.";
    case ToolType::Gradient:        return "Trace un dégradé entre la couleur primaire et secondaire.";
    case ToolType::Brush:           return "Peint des traits doux avec la couleur active.";
    case ToolType::Eraser:          return "Efface vers la transparence (ou la couleur secondaire).";
    case ToolType::Pencil:          return "Dessine des pixels nets, sans anticrénelage, en 1 px.";
    case ToolType::ColorPicker:     return "Prélève une couleur de l'image comme couleur active.";
    case ToolType::CloneStamp:      return "Duplique une partie de l'image (Ctrl+clic définit la source).";
    case ToolType::Recolor:         return "Remplace une couleur par la couleur active là où l'on peint.";
    case ToolType::Text:            return "Ajoute du texte sur l'image.";
    case ToolType::Line:            return "Trace des lignes et des courbes.";
    case ToolType::Shape:           return "Dessine des formes (rectangle, ellipse, étoile...).";
    default:                        return QString();
    }
}

void MainWindow::createToolsPanel() {
    // Tools panel: 2 columns like paint.net
    // Col 0 (left):  RectSel, Lasso, EllipseSel, MagicWand, Fill, Brush, Pencil, CloneStamp, Text, Shape
    // Col 1 (right): Move, MoveSelection, Zoom, Pan, Gradient, Eraser, ColorPicker, Recolor, Line
    m_toolActionGroup = new QActionGroup(this);
    m_toolActionGroup->setExclusive(true);

    struct ToolDef {
        QString name;
        QString shortcut;
        ToolType type;
        int row;
        int col;
    };

    // paint.net's Tools window, 2 columns x 10 rows — positions taken from the
    // official Tools window (paint.net/doc/latest/images/tools/toolswindow.png):
    // the four SELECTION tools run down the LEFT column (rows 0-3) while
    // Move / Move Selection / Zoom / Pan run down the RIGHT column (rows 0-3);
    // the remaining groups sit in pairs on rows 4-9.
    // The list order below is paint.net's documented tool order (1..19) — it is what
    // the single-row horizontal palette follows.
    const QVector<ToolDef> toolDefs = {
        // Selection tools -> left column
        {"Sélection rectangle",              "S",  ToolType::RectSelection,    0, 0},
        {"Lasso de sélection",               "S",  ToolType::LassoSelection,   1, 0},
        {"Sélection ellipse",                "S",  ToolType::EllipseSelection, 2, 0},
        {"Baguette magique",                 "S",  ToolType::MagicWand,        3, 0},
        // Move + view tools -> right column
        {"Déplacer les pixels sélectionnés", "M",  ToolType::Move,             0, 1},
        {"Déplacer la sélection",            "M",  ToolType::MoveSelection,    1, 1},
        {"Zoom / Loupe",                     "Z",  ToolType::Zoom,             2, 1},
        {"Se déplacer dans l'image",         "H",  ToolType::Pan,              3, 1},
        // Fill tools
        {"Pot de peinture",                  "F",  ToolType::Fill,             4, 0},
        {"Dégradé",                          "G",  ToolType::Gradient,         4, 1},
        // Drawing tools
        {"Pinceau",                          "B",  ToolType::Brush,            5, 0},
        {"Gomme",                            "E",  ToolType::Eraser,           5, 1},
        {"Crayon",                           "P",  ToolType::Pencil,           6, 0},
        // Photo tools
        {"Sélecteur de couleur",             "K",  ToolType::ColorPicker,      6, 1},
        {"Tampon de clonage",                "L",  ToolType::CloneStamp,       7, 0},
        {"Recoloriage",                      "R",  ToolType::Recolor,          7, 1},
        // Text and shape tools
        {"Texte",                            "T",  ToolType::Text,             8, 0},
        {"Ligne / Courbe",                   "O",  ToolType::Line,             8, 1},
        {"Formes",                           "O",  ToolType::Shape,            9, 0},
    };

    auto *toolsWidget = new QWidget;
    toolsWidget->setObjectName("ToolsPalette");
    auto *grid = new QGridLayout(toolsWidget);
    grid->setSpacing(0);
    grid->setContentsMargins(1, 2, 1, 2);

    for (const auto &td : toolDefs) {
        auto *btn = new QToolButton;
        btn->setIcon(ToolIcons::forTool(td.type));
        btn->setIconSize(QSize(20, 20));
        // Fill the column width so the two columns cover the whole dock (the
        // title bar makes the dock a little wider than 2x28 — letting the
        // buttons expand removes the empty strip on the right).
        btn->setFixedHeight(28);
        btn->setMinimumWidth(28);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        // Rich tooltip: bold name + shortcut, then a one-line description.
        QString tip = "<b>" + TR(td.name) + "</b>";
        if (!td.shortcut.isEmpty()) tip += " (" + td.shortcut + ")";
        const QString desc = toolDescription(td.type);
        if (!desc.isEmpty()) tip += "<br>" + TR(desc);
        btn->setToolTip(tip);
        // Remember the untranslated name + key so tooltips can be rebuilt on a
        // language change.
        btn->setProperty("toolName", td.name);
        btn->setProperty("toolKey", td.shortcut);
        btn->setProperty("toolTypeInt", static_cast<int>(td.type));
        // paint.net's grid slot, used by the vertical (2-column) palette layout.
        btn->setProperty("pdnRow", td.row);
        btn->setProperty("pdnCol", td.col);
        m_toolButtons.append(btn);
        // NB: no btn->setShortcut() here — the single-key shortcuts are owned by
        // the global actions in createKeyboardShortcuts(); binding them twice
        // caused "Ambiguous shortcut overload" and neither would fire.

        ToolType type = td.type;
        connect(btn, &QToolButton::clicked, this, [this, type]() {
            selectTool(type);
        });

        auto *action = new QAction(td.name, this);
        action->setCheckable(true);
        m_toolActionGroup->addAction(action);
        m_toolActions.insert(type, action);
        connect(action, &QAction::toggled, btn, &QToolButton::setChecked);
        connect(btn, &QToolButton::clicked, action, [action]() { action->setChecked(true); });

        grid->addWidget(btn, td.row, td.col);
    }
    m_toolsGrid = grid;
    m_toolsPaletteWidget = toolsWidget;
    layoutToolsPalette(false);   // vertical (2 columns) by default

    m_toolsDock = new QDockWidget(TR("Outils"), this);
    m_toolsDock->setObjectName("ToolsDock");
    m_toolsDock->setWidget(toolsWidget);
    m_toolsDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_toolsDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolsDock);
    // When docked to the top or bottom, lay the tools out horizontally (2 rows,
    // left-to-right); otherwise keep the vertical 2-column palette.
    connect(m_toolsDock, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea area) {
        const bool horizontal = (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea);
        layoutToolsPalette(horizontal);
        // Setting the widget's max height isn't enough — force the dock itself to
        // collapse to the single-row height so there's no empty band above/below.
        if (horizontal)
            resizeDocks({m_toolsDock}, {30}, Qt::Vertical);
        normalizeDockLayout(m_toolsDock);
    });
    connect(m_toolsDock, &QDockWidget::topLevelChanged, this, [this](bool floating) {
        if (floating) layoutToolsPalette(false);   // floating -> vertical
    });
}

void MainWindow::layoutToolsPalette(bool horizontal) {
    if (!m_toolsGrid || !m_toolsPaletteWidget) return;

    // Detach all buttons from the grid.
    for (QToolButton *btn : m_toolButtons)
        if (btn) m_toolsGrid->removeWidget(btn);
    // Clear old stretches.
    for (int i = 0; i < 20; ++i) {
        m_toolsGrid->setRowStretch(i, 0);
        m_toolsGrid->setColumnStretch(i, 0);
    }

    const int n = m_toolButtons.size();
    int maxRow = 0;
    for (int i = 0; i < n; ++i) {
        int row, col;
        if (horizontal) {
            row = 0; col = i;                 // single row, left-to-right
        } else {
            // Exact paint.net slot for this tool (selection tools down the left
            // column, move/view down the right), not a plain row-by-row fill.
            row = m_toolButtons[i]->property("pdnRow").toInt();
            col = m_toolButtons[i]->property("pdnCol").toInt();
            maxRow = qMax(maxRow, row);
        }
        m_toolsGrid->addWidget(m_toolButtons[i], row, col);
    }

    if (horizontal) {
        // One row hugging the top: kill the vertical margins and pin the height to
        // the icon row so the dock has no empty band above/below.
        m_toolsGrid->setContentsMargins(1, 0, 1, 0);
        const int rowH = 28;   // button height
        m_toolsPaletteWidget->setMinimumWidth(0);
        m_toolsPaletteWidget->setMaximumWidth(QWIDGETSIZE_MAX);
        m_toolsPaletteWidget->setMinimumHeight(rowH);
        m_toolsPaletteWidget->setMaximumHeight(rowH);   // exact height, no slack
        // Clear the sticky tall minimum normalizeDockLayout imposes for the vertical
        // palette — otherwise the dock can't shrink to the single row.
        if (m_toolsDock) m_toolsDock->setMinimumSize(0, 0);
        for (int c = 0; c < n; ++c) m_toolsGrid->setColumnStretch(c, 0);
        m_toolsGrid->setColumnStretch(n, 1);   // trailing column takes the slack
        m_toolsGrid->setRowStretch(0, 0);
    } else {
        // Two columns; absorb slack at the bottom.
        m_toolsGrid->setContentsMargins(1, 2, 1, 2);
        m_toolsPaletteWidget->setMinimumHeight(0);
        m_toolsPaletteWidget->setMaximumHeight(QWIDGETSIZE_MAX);
        m_toolsPaletteWidget->setMinimumWidth(28 * 2 + 2);
        m_toolsPaletteWidget->setMaximumWidth(200);
        if (m_toolsDock) m_toolsDock->setMinimumSize(0, 0);
        m_toolsGrid->setColumnStretch(0, 1);
        m_toolsGrid->setColumnStretch(1, 1);
        m_toolsGrid->setRowStretch(maxRow + 1, 1);   // slack below the last tool row
    }
}

void MainWindow::createDockPanels() {
    // Tool Options as horizontal toolbar under the main toolbar
    m_toolOptionsPanel = new ToolOptionsPanel;
    connect(m_toolOptionsPanel, &ToolOptionsPanel::toolChangeRequested,
            this, &MainWindow::selectTool);
    connect(m_toolOptionsPanel, &ToolOptionsPanel::toolOptionsChanged,
            this, [this]() { if (m_canvas) m_canvas->update(); });
    auto *optionsBar = new QToolBar("Barre d'outils variable", this);
    optionsBar->setObjectName("VariableToolbar");
    optionsBar->setMovable(false);
    optionsBar->setIconSize(QSize(14, 14));
    optionsBar->addWidget(m_toolOptionsPanel);
    addToolBar(Qt::TopToolBarArea, optionsBar);
    m_variableToolbar = optionsBar;

    // Colors panel - LEFT side (below Tools)
    m_colorsDock = new QDockWidget(TR("Couleurs"), this);
    m_colorsDock->setObjectName("ColorsDock");
    m_colorsDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_colorsDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_colorsPanel = new ColorsPanel;
    m_colorsPanel->setDocument(m_document);
    m_colorsDock->setWidget(m_colorsPanel);
    addDockWidget(Qt::LeftDockWidgetArea, m_colorsDock);
    connect(m_colorsDock, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) {
        normalizeDockLayout(m_colorsDock);
    });

    // History panel - RIGHT top
    m_historyDock = new QDockWidget(TR("Historique"), this);
    m_historyDock->setObjectName("HistoryDock");
    m_historyDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_historyDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_historyPanel = new HistoryPanel;
    m_historyPanel->setDocument(m_document);
    m_historyDock->setWidget(m_historyPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_historyDock);
    connect(m_historyDock, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) {
        normalizeDockLayout(m_historyDock);
    });

    // Layers panel - RIGHT bottom
    m_layersDock = new QDockWidget(TR("Calques"), this);
    m_layersDock->setObjectName("LayersDock");
    m_layersDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_layersDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_layersPanel = new LayersPanel;
    m_layersPanel->setDocument(m_document);
    m_layersDock->setWidget(m_layersPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_layersDock);
    connect(m_layersDock, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) {
        normalizeDockLayout(m_layersDock);
    });

    m_toolsDock->setFloating(true);
    m_historyDock->setFloating(true);
    m_layersDock->setFloating(true);
    m_colorsDock->setFloating(true);

    m_toolsDock->resize(62, 520);
    m_historyDock->resize(260, 200);
    m_layersDock->resize(260, 220);
    m_colorsDock->resize(220, 320);

    QTimer::singleShot(0, this, [this]() {
        QSettings settings("PaintDali", "PaintDali");
        if (settings.contains("ui/mainWindowStateV2")) {
            // A saved layout exists; do not overwrite it with default startup positions.
            return;
        }

        const QRect g = geometry();
        m_toolsDock->move(g.left() + 6, g.top() + 115);
        m_historyDock->move(g.right() - m_historyDock->width() - 10, g.top() + 115);
        m_layersDock->move(g.right() - m_layersDock->width() - 10, g.bottom() - m_layersDock->height() - 40);
        m_colorsDock->move(g.left() + 6, g.bottom() - m_colorsDock->height() - 40);
        normalizeDockLayout(m_toolsDock);
        normalizeDockLayout(m_historyDock);
        normalizeDockLayout(m_layersDock);
        normalizeDockLayout(m_colorsDock);
    });
}

void MainWindow::createStatusBar() {
    // Active tool name on the far left.
    m_toolLabel = new QLabel("Pinceau");
    m_toolLabel->setStyleSheet("font-size: 11px; font-weight: bold; padding-left: 4px; padding-right: 6px;");
    statusBar()->addWidget(m_toolLabel);

    // Help text on the left (like Paint.NET)
    m_helpTextLabel = new QLabel(TR("Clic gauche pour dessiner avec la couleur primaire, clic droit avec la couleur secondaire."));
    m_helpTextLabel->setStyleSheet("font-size: 11px; padding-left: 4px;");

    // Size icon (drawn grid icon) + dimensions
    auto *sizeIcon = new QLabel;
    {
        QPixmap pm(12, 12);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setPen(QPen(QColor(100, 100, 100), 1));
        p.drawRect(1, 1, 10, 10);
        p.drawLine(1, 6, 11, 6);
        p.drawLine(6, 1, 6, 11);
        p.end();
        sizeIcon->setPixmap(pm);
    }
    sizeIcon->setFixedWidth(14);
    m_sizeLabel = new QLabel("800 × 600");
    m_sizeLabel->setStyleSheet("font-size: 11px;");

    // Cursor position icon (drawn crosshair) + coords
    auto *cursorIcon = new QLabel;
    {
        QPixmap pm(12, 12);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(QColor(100, 100, 100), 1));
        p.drawLine(6, 1, 6, 11);
        p.drawLine(1, 6, 11, 6);
        p.drawEllipse(3, 3, 6, 6);
        p.end();
        cursorIcon->setPixmap(pm);
    }
    cursorIcon->setFixedWidth(14);
    m_positionLabel = new QLabel("0, 0");
    m_positionLabel->setStyleSheet("font-size: 11px;");

    // "px" unit label
    auto *pxLabel = new QLabel("px");
    pxLabel->setStyleSheet("font-size: 11px; color: #666;");

    // Zoom percentage
    m_zoomLabel = new QLabel("100%");
    m_zoomLabel->setStyleSheet("font-size: 11px;");
    m_zoomLabel->setFixedWidth(42);

    // Paint.NET-like zoom controls on the right: [-] [slider] [+]
    auto *zoomOutBtn = new QToolButton;
    zoomOutBtn->setText("-");
    zoomOutBtn->setFixedSize(18, 16);
    zoomOutBtn->setToolTip(TR("Zoom arrière"));

    auto *zoomSlider = new QSlider(Qt::Horizontal);
    zoomSlider->setFixedWidth(92);
    zoomSlider->setFixedHeight(16);

    auto *zoomInBtn = new QToolButton;
    zoomInBtn->setText("+");
    zoomInBtn->setFixedSize(18, 16);
    zoomInBtn->setToolTip(TR("Zoom avant"));

    static const int zoomSteps[] = {
        1, 2, 3, 5, 8, 12, 16, 25, 33, 50, 66, 75,
        100, 125, 150, 200, 300, 400, 600, 800,
        1200, 1600, 2400, 3200, 4800, 6400
    };
    const int zoomStepCount = static_cast<int>(sizeof(zoomSteps) / sizeof(zoomSteps[0]));
    zoomSlider->setRange(0, zoomStepCount - 1);

    auto findClosestZoomIndex = [zoomStepCount](int percent) {
        int bestIdx = 0;
        int bestDelta = std::abs(zoomSteps[0] - percent);
        for (int i = 1; i < zoomStepCount; ++i) {
            int d = std::abs(zoomSteps[i] - percent);
            if (d < bestDelta) {
                bestDelta = d;
                bestIdx = i;
            }
        }
        return bestIdx;
    };

    int initialPercent = static_cast<int>(m_canvas->zoom() * 100.0);
    zoomSlider->setValue(findClosestZoomIndex(initialPercent));

    connect(zoomOutBtn, &QToolButton::clicked, this, [this]() { m_canvas->zoomOut(); });
    connect(zoomInBtn, &QToolButton::clicked, this, [this]() { m_canvas->zoomIn(); });

    connect(zoomSlider, &QSlider::valueChanged, this, [this](int idx) {
        m_canvas->setZoom(zoomSteps[idx] / 100.0);
    });

    connect(m_canvas, &CanvasWidget::zoomChanged, this, [this, zoomSlider, findClosestZoomIndex](double zoom) {
        int percent = static_cast<int>(zoom * 100.0);
        m_zoomLabel->setText(QString("%1%").arg(percent));
        zoomSlider->blockSignals(true);
        zoomSlider->setValue(findClosestZoomIndex(percent));
        zoomSlider->blockSignals(false);
    });

    statusBar()->addWidget(m_helpTextLabel, 1);
    statusBar()->addPermanentWidget(sizeIcon);
    statusBar()->addPermanentWidget(m_sizeLabel);
    statusBar()->addPermanentWidget(cursorIcon);
    statusBar()->addPermanentWidget(m_positionLabel);
    statusBar()->addPermanentWidget(pxLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->addPermanentWidget(zoomOutBtn);
    statusBar()->addPermanentWidget(zoomSlider);
    statusBar()->addPermanentWidget(zoomInBtn);
}

// ---- File operations ----

void MainWindow::wireDocumentSignals() {
    m_canvas->setDocument(m_document);
    m_layersPanel->setDocument(m_document);
    m_colorsPanel->setDocument(m_document);
    m_historyPanel->setDocument(m_document);

    connect(m_document, &Document::documentChanged, this, &MainWindow::updateTitle);
    connect(m_document, &Document::documentChanged, this, &MainWindow::updateImageList);
    connect(m_document, &Document::sizeChanged, this, [this](int w, int h) {
        m_sizeLabel->setText(QString("%1 × %2").arg(w).arg(h));
    });
    connect(&m_document->history(), &HistoryManager::historyChanged, this, [this]() {
        const bool canUndo = m_document->history().canUndo();
        const bool canRedo = m_document->history().canRedo();
        m_undoAction->setEnabled(canUndo);
        m_redoAction->setEnabled(canRedo);
        if (m_undoToolbarAction) m_undoToolbarAction->setEnabled(canUndo);
        if (m_redoToolbarAction) m_redoToolbarAction->setEnabled(canRedo);
    });

    m_canvas->resetToDefaultView();
    m_sizeLabel->setText(QString("%1 × %2").arg(m_document->width()).arg(m_document->height()));
    updateTitle();
}

// ---- Multi-document ----

void MainWindow::addDocument(Document *doc) {
    m_documents.append(doc);
    setActiveDocument(m_documents.size() - 1);
}

void MainWindow::setActiveDocument(int index) {
    if (index < 0 || index >= m_documents.size()) return;
    // Drop the previous document's connections so stale signals don't fire.
    if (m_document) disconnect(m_document, nullptr, this, nullptr);

    m_activeDocIndex = index;
    m_document = m_documents[index];
    wireDocumentSignals();

    // Refresh undo/redo enablement for the newly active document.
    const bool canUndo = m_document->history().canUndo();
    const bool canRedo = m_document->history().canRedo();
    m_undoAction->setEnabled(canUndo);
    m_redoAction->setEnabled(canRedo);
    if (m_undoToolbarAction) m_undoToolbarAction->setEnabled(canUndo);
    if (m_redoToolbarAction) m_redoToolbarAction->setEnabled(canRedo);

    updateImageList();
}

void MainWindow::closeDocument(int index) {
    if (index < 0 || index >= m_documents.size()) return;
    Document *doc = m_documents[index];
    if (!maybeSaveDocument(doc)) return;

    if (m_document == doc) disconnect(m_document, nullptr, this, nullptr);
    m_documents.removeAt(index);
    doc->deleteLater();

    if (m_documents.isEmpty()) {
        // paint.net always keeps at least one image open.
        auto *fresh = new Document(800, 600, this);
        fresh->activeLayer()->clear(Qt::white);
        m_document = nullptr;
        addDocument(fresh);
        return;
    }
    m_document = nullptr;
    setActiveDocument(qBound(0, index - 1, m_documents.size() - 1));
}

void MainWindow::closeCurrentDocument() {
    closeDocument(m_activeDocIndex);
}

void MainWindow::updateImageList() {
    if (m_imageListBar)
        m_imageListBar->setDocuments(m_documents, m_activeDocIndex);
    updateTitle();
}

bool MainWindow::maybeSaveDocument(Document *doc) {
    if (!doc || !doc->isModified()) return true;
    // Make sure the user is looking at the document they're being asked about.
    int idx = m_documents.indexOf(doc);
    if (idx >= 0 && idx != m_activeDocIndex) setActiveDocument(idx);

    auto result = QMessageBox::question(this, TR("Enregistrer les modifications"),
        TR("Le document a été modifié.\nVoulez-vous enregistrer les modifications ?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (result == QMessageBox::Save) return saveDocument();
    if (result == QMessageBox::Cancel) return false;
    return true;  // Discard
}

bool MainWindow::maybeSave() {
    return maybeSaveDocument(m_document);
}

void MainWindow::newDocument() {
    NewDocumentDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        auto *newDoc = new Document(dlg.imageWidth(), dlg.imageHeight(), this);
        newDoc->activeLayer()->clear(dlg.backgroundColor());
        newDoc->setModified(false);
        addDocument(newDoc);   // opens alongside the existing images
    }
}

void MainWindow::openDocument() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Ouvrir une image",
        QString(), "Tous les formats pris en charge (*.psw *.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp);;"
                   "paint.software (calques) (*.psw);;"
                   "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp);;All Files (*)");
    for (const QString &f : files)
        loadDocumentInto(f);
}

QString MainWindow::autosaveDir() const {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.paintdali";
    return base + "/recovery";
}

void MainWindow::performAutosave() {
    if (!m_document || !m_document->isModified()) return;
    QDir().mkpath(autosaveDir());
    // Flattened recovery snapshot (layered .pdn isn't implemented yet) + a
    // sidecar recording the original path so we can label it on recovery.
    const QString png = autosaveDir() + "/autosave.png";
    QImage flat = m_document->flatten();
    if (flat.save(png, "PNG")) {
        QSettings s("PaintDali", "PaintDali");
        s.setValue("recovery/pending", true);
        s.setValue("recovery/origPath", m_document->filePath());
        s.setValue("recovery/when", QDateTime::currentDateTime().toString(Qt::ISODate));
    }
}

void MainWindow::clearAutosave() {
    QFile::remove(autosaveDir() + "/autosave.png");
    QSettings s("PaintDali", "PaintDali");
    s.remove("recovery/pending");
    s.remove("recovery/origPath");
    s.remove("recovery/when");
}

void MainWindow::checkForRecovery() {
    QSettings s("PaintDali", "PaintDali");
    if (!s.value("recovery/pending", false).toBool()) return;
    const QString png = autosaveDir() + "/autosave.png";
    if (!QFile::exists(png)) { clearAutosave(); return; }

    const QString orig = s.value("recovery/origPath").toString();
    const QString when = s.value("recovery/when").toString();
    const QString what = orig.isEmpty() ? TR("un document sans titre") : QFileInfo(orig).fileName();

    auto r = QMessageBox::question(this, TR("Récupération"),
        TR("Une sauvegarde automatique a été trouvée (%1, %2).\n"
           "Voulez-vous la récupérer ?").arg(what, when),
        QMessageBox::Yes | QMessageBox::No);
    if (r == QMessageBox::Yes) {
        loadDocumentInto(png);
        if (m_document) {
            m_document->setModified(true);   // force a re-save so it isn't lost again
            updateTitle();
        }
    } else {
        clearAutosave();
    }
}

void MainWindow::openFile(const QString &filePath) {
    loadDocumentInto(filePath);
    // If the only other image is a pristine untitled one, drop it — paint.net
    // doesn't leave an empty canvas hanging around when you open a file.
    if (m_documents.size() == 2) {
        Document *first = m_documents[0];
        if (first->filePath().isEmpty() && !first->isModified() && first != m_document) {
            m_documents.removeAt(0);
            first->deleteLater();
            m_activeDocIndex = 0;
            updateImageList();
        }
    }
}

void MainWindow::loadDocumentInto(const QString &filePath) {
    // If it's already open, just switch to it.
    for (int i = 0; i < m_documents.size(); ++i) {
        if (m_documents[i]->filePath() == filePath) {
            setActiveDocument(i);
            return;
        }
    }
    auto *newDoc = new Document(1, 1, this);
    if (newDoc->load(filePath)) {
        addDocument(newDoc);
        addRecentFile(filePath);
    } else {
        delete newDoc;
        QMessageBox::warning(this, TR("Erreur"), TR("Impossible d'ouvrir le fichier image."));
    }
}

void MainWindow::printDocument() {
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle(TR("Imprimer l'image"));
    if (dlg.exec() != QDialog::Accepted) return;

    QPainter painter(&printer);
    QImage image = m_document->flatten();
    QRect page = painter.viewport();
    QSize size = image.size();
    size.scale(page.size(), Qt::KeepAspectRatio);
    painter.setViewport(page.x(), page.y(), size.width(), size.height());
    painter.setWindow(image.rect());
    painter.drawImage(0, 0, image);
}

void MainWindow::openRecentFile() {
    auto *action = qobject_cast<QAction*>(sender());
    if (!action) return;
    if (!maybeSave()) return;
    loadDocumentInto(action->data().toString());
}

void MainWindow::addRecentFile(const QString &filePath) {
    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);
    while (m_recentFiles.size() > 10) m_recentFiles.removeLast();
    QSettings().setValue("recentFiles", m_recentFiles);
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu() {
    if (!m_recentFilesMenu) return;
    m_recentFilesMenu->clear();
    if (m_recentFiles.isEmpty()) {
        m_recentFilesMenu->addAction(TR("(aucun)"))->setEnabled(false);
        return;
    }
    for (const QString &path : m_recentFiles) {
        QAction *a = m_recentFilesMenu->addAction(QFileInfo(path).fileName(),
                                                  this, &MainWindow::openRecentFile);
        a->setData(path);
        a->setToolTip(path);
    }
    m_recentFilesMenu->addSeparator();
    m_recentFilesMenu->addAction(TR("Effacer la liste"), this, [this]() {
        m_recentFiles.clear();
        QSettings().setValue("recentFiles", m_recentFiles);
        updateRecentFilesMenu();
    });
}

bool MainWindow::saveDocument() {
    if (m_document->filePath().isEmpty())
        return saveDocumentAs();
    if (!m_document->save(m_document->filePath())) {
        QMessageBox::warning(this, TR("Erreur"), TR("Impossible d'enregistrer le fichier."));
        return false;
    }
    addRecentFile(m_document->filePath());
    updateTitle();
    return true;
}

bool MainWindow::saveDocumentAs() {
    QString selectedFilter;
    QString filePath = QFileDialog::getSaveFileName(this, "Enregistrer l'image",
        QString(), "paint.software - garde les calques (*.psw);;PNG (*.png);;JPEG (*.jpg *.jpeg);;"
                   "BMP (*.bmp);;TIFF (*.tiff);;WebP (*.webp);;All Files (*)",
        &selectedFilter);
    if (filePath.isEmpty()) return false;

    // Ensure the file has an extension; infer it from the chosen filter.
    if (QFileInfo(filePath).suffix().isEmpty()) {
        QString ext = "png";
        if (selectedFilter.contains("psw")) ext = "psw";
        else if (selectedFilter.contains("jpg")) ext = "jpg";
        else if (selectedFilter.contains("bmp")) ext = "bmp";
        else if (selectedFilter.contains("tiff")) ext = "tiff";
        else if (selectedFilter.contains("webp")) ext = "webp";
        filePath += "." + ext;
    }

    // Warn when a multi-layer document is saved to a flat image format.
    if (!Document::isNativeFormat(filePath) && m_document->layerCount() > 1) {
        auto res = QMessageBox::question(this, TR("Aplatir les calques ?"),
            TR("Ce format ne conserve pas les calques : l'image sera aplatie.\n"
               "Utilisez le format .psw pour garder vos calques.\n\nContinuer ?"),
            QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save);
        if (res == QMessageBox::Cancel) return false;
    }

    if (!m_document->save(filePath)) {
        QMessageBox::warning(this, TR("Erreur"), TR("Impossible d'enregistrer le fichier."));
        return false;
    }
    addRecentFile(filePath);
    updateTitle();
    return true;
}

// ---- Edit operations ----

void MainWindow::undo() { m_document->history().undo(); m_canvas->updateCanvas(); }
void MainWindow::redo() { m_document->history().redo(); m_canvas->updateCanvas(); }

void MainWindow::cut() {
    copy();
    deleteSelectionContents();
}

void MainWindow::copy() {
    if (!m_document->activeLayer()) return;
    QImage img = m_document->activeLayer()->image();
    if (m_document->selection().hasSelection()) {
        img = m_document->selection().getMaskedImage(img, m_document->activeLayer()->offset());
    }
    QApplication::clipboard()->setImage(img);
}

void MainWindow::copyMerged() {
    if (!m_document) return;
    // Copy the flattened (all visible layers) image, cropped to the selection.
    QImage img = m_document->flatten();
    if (m_document->selection().hasSelection())
        img = m_document->selection().getMaskedImage(img);
    QApplication::clipboard()->setImage(img);
}

void MainWindow::paste() {
    QImage img = QApplication::clipboard()->image();
    if (img.isNull()) return;
    m_document->addLayer(img, "Calque collé");
    m_canvas->updateCanvas();
}

void MainWindow::pasteIntoNewLayer() {
    // In this implementation regular paste already lands on a new layer; this
    // menu item makes the intent explicit and is what paint.net users expect.
    paste();
}

void MainWindow::selectAll() {
    m_document->selection().selectAll();
    emit m_document->selectionChanged();
    m_canvas->update();
}

void MainWindow::deselectAll() {
    m_document->selection().clear();
    emit m_document->selectionChanged();
    m_canvas->update();
}

void MainWindow::invertSelection() {
    m_document->selection().invert();
    emit m_document->selectionChanged();
    m_canvas->update();
}

// ---- Image operations ----

void MainWindow::resizeImage() {
    ResizeDialog dlg(m_document->width(), m_document->height(), 96, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_document->resize(dlg.newWidth(), dlg.newHeight());
        m_sizeLabel->setText(QString("%1 × %2").arg(m_document->width()).arg(m_document->height()));
        m_canvas->updateCanvas();
    }
}

void MainWindow::canvasSize() {
    CanvasSizeDialog dlg(m_document->width(), m_document->height(), this);
    if (dlg.exec() == QDialog::Accepted) {
        m_document->resizeCanvas(dlg.newWidth(), dlg.newHeight(), dlg.anchorX(), dlg.anchorY());
        m_sizeLabel->setText(QString("%1 × %2").arg(m_document->width()).arg(m_document->height()));
        m_canvas->updateCanvas();
    }
}

void MainWindow::applyImageOperation(const QImage &result, const QString &description) {
    auto *layer = m_document->activeLayer();
    if (!layer) return;
    QImage before = layer->image().copy();
    layer->setImage(result);
    m_document->pushImageEdit(m_document->activeLayerIndex(), before, description);
    m_canvas->updateCanvas();
}

void MainWindow::applyImageOperationToTargetLayers(const std::function<QImage(const QImage &)> &operation, const QString &description) {
    if (!m_document) return;

    // Remember this operation (with its parameters bound) so "Repeat last effect"
    // can re-apply it without re-prompting.
    m_lastEffect = [this, operation, description]() {
        applyImageOperationToTargetLayers(operation, description);
    };
    m_lastEffectLabel = description;
    if (m_repeatEffectAction) {
        m_repeatEffectAction->setEnabled(true);
        m_repeatEffectAction->setText(TR("Répéter : ") + description);
    }

    // When a selection is active, confine the effect to it (paint.net behaviour).
    // Only do this for size-preserving operations; generative effects that
    // return a different size fall through to the whole-layer path.
    if (m_document->selection().hasSelection()) {
        applySelectionFilterToTargetLayers(operation, description);
        return;
    }

    bool changed = false;
    for (int index : targetLayerIndices()) {
        auto *layer = m_document->layerAt(index);
        if (!layer) continue;

        QImage before = layer->image().copy();
        QImage after = operation(before);
        // Compare in the same format: adjustments hand back straight-alpha
        // ARGB32 while the layer is premultiplied, so a raw != would always be
        // true and push a redundant undo step for an identity operation.
        if (after.format() != before.format())
            after = after.convertToFormat(before.format());
        if (after.size() != before.size() || after != before) {
            layer->setImage(after);
            m_document->pushImageEdit(index, before, description);
            changed = true;
        }
    }

    if (changed) {
        m_canvas->updateCanvas();
    }
}

void MainWindow::applySelectionFilterToTargetLayers(const std::function<QImage(const QImage &)> &operation, const QString &description) {
    if (!m_document || !m_document->selection().hasSelection()) return;

    const QRect selectionRect = m_document->selection().boundingRect();
    bool changed = false;

    for (int index : targetLayerIndices()) {
        auto *layer = m_document->layerAt(index);
        if (!layer) continue;

        QRect rect = selectionRect.translated(-layer->offset()).intersected(layer->image().rect());
        if (rect.isEmpty()) continue;

        QImage before = layer->image().copy();
        QImage original = layer->image().copy(rect).convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QImage processed = operation(original).convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (processed.size() != original.size()) continue;

        QImage mask = m_document->selection().maskForImage(layer->image().size(), layer->offset()).copy(rect).convertToFormat(QImage::Format_Grayscale8);
        for (int y = 0; y < rect.height(); ++y) {
            QRgb *destLine = reinterpret_cast<QRgb*>(layer->image().scanLine(rect.y() + y)) + rect.x();
            const QRgb *srcLine = reinterpret_cast<const QRgb*>(original.constScanLine(y));
            const QRgb *fxLine = reinterpret_cast<const QRgb*>(processed.constScanLine(y));
            const uchar *maskLine = mask.constScanLine(y);

            for (int x = 0; x < rect.width(); ++x) {
                const int amount = maskLine[x];
                if (amount == 0) continue;

                const QRgb src = srcLine[x];
                const QRgb fx = fxLine[x];
                const int inv = 255 - amount;
                destLine[x] = qRgba(
                    (qRed(src) * inv + qRed(fx) * amount) / 255,
                    (qGreen(src) * inv + qGreen(fx) * amount) / 255,
                    (qBlue(src) * inv + qBlue(fx) * amount) / 255,
                    (qAlpha(src) * inv + qAlpha(fx) * amount) / 255);
            }
        }

        if (layer->image() != before) {
            m_document->pushImageEdit(index, before, description);
            changed = true;
        }
    }

    if (changed) {
        m_canvas->updateCanvas();
    }
}

void MainWindow::transformSelectionContents(const std::function<QImage(const QImage &)> &operation, const QString &description) {
    if (!m_document || !m_document->selection().hasSelection()) return;

    const QRect selectionRect = m_document->selection().boundingRect();
    bool changed = false;

    for (int index : targetLayerIndices()) {
        auto *layer = m_document->layerAt(index);
        if (!layer) continue;

        QRect rect = selectionRect.translated(-layer->offset()).intersected(layer->image().rect());
        if (rect.isEmpty()) continue;

        QImage before = layer->image().copy();
        QImage source = layer->image().copy(rect).convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QImage mask = m_document->selection().maskForImage(layer->image().size(), layer->offset()).copy(rect).convertToFormat(QImage::Format_Grayscale8);

        for (int y = 0; y < rect.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(source.scanLine(y));
            const uchar *maskLine = mask.constScanLine(y);
            for (int x = 0; x < rect.width(); ++x) {
                line[x] = qRgba(qRed(line[x]), qGreen(line[x]), qBlue(line[x]), qAlpha(line[x]) * maskLine[x] / 255);
            }
        }

        QImage transformed = operation(source).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_document->selection().eraseFromImage(layer->image(), layer->offset());

        QRect targetRect(QPoint(0, 0), transformed.size());
        targetRect.moveCenter(rect.center());
        QPainter painter(&layer->image());
        painter.drawImage(targetRect.topLeft(), transformed);

        if (layer->image() != before) {
            m_document->pushImageEdit(index, before, description);
            changed = true;
        }
    }

    if (changed) {
        m_canvas->updateCanvas();
    }
}

void MainWindow::deleteSelectionContents() {
    if (!m_document || !m_document->selection().hasSelection()) return;

    bool changed = false;
    for (int index : targetLayerIndices()) {
        auto *layer = m_document->layerAt(index);
        if (!layer) continue;

        QImage before = layer->image().copy();
        m_document->selection().eraseFromImage(layer->image(), layer->offset());
        if (layer->image() != before) {
            m_document->pushImageEdit(index, before, "Supprimer la selection");
            changed = true;
        }
    }

    if (changed) {
        m_canvas->updateCanvas();
    }
}

void MainWindow::showSelectionContextMenu(const QPoint &globalPos) {
    if (!m_document || !m_document->selection().hasSelection()) return;

    QMenu menu(this);
    menu.addAction(TR("Supprimer"), this, &MainWindow::deleteSelectionContents);
    menu.addAction(TR("Couper"), this, &MainWindow::cut);
    menu.addAction(TR("Copier"), this, &MainWindow::copy);
    menu.addSeparator();
    menu.addAction(TR("Rotation 90° horaire"), this, [this]() {
        QTransform t;
        t.rotate(90);
        transformSelectionContents([t](const QImage &image) {
            return image.transformed(t, Qt::SmoothTransformation);
        }, "Rotation selection 90° CW");
    });
    menu.addAction(TR("Rotation 90° anti-horaire"), this, [this]() {
        QTransform t;
        t.rotate(-90);
        transformSelectionContents([t](const QImage &image) {
            return image.transformed(t, Qt::SmoothTransformation);
        }, "Rotation selection 90° CCW");
    });
    menu.addAction(TR("Retourner horizontalement"), this, [this]() {
        transformSelectionContents([](const QImage &image) {
            return image.mirrored(true, false);
        }, "Retourner selection horizontalement");
    });
    menu.addAction(TR("Retourner verticalement"), this, [this]() {
        transformSelectionContents([](const QImage &image) {
            return image.mirrored(false, true);
        }, "Retourner selection verticalement");
    });

    auto *effectsMenu = menu.addMenu(TR("Effets rapides"));
    effectsMenu->addAction(TR("Inverser les couleurs"), this, [this]() {
        InvertColors adj;
        applySelectionFilterToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, "Inverser couleurs selection");
    });
    effectsMenu->addAction(TR("Noir et blanc"), this, [this]() {
        Desaturate adj;
        applySelectionFilterToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, "Noir et blanc selection");
    });
    effectsMenu->addAction(TR("Sépia"), this, [this]() {
        Sepia adj;
        adj.setIntensity(80);
        applySelectionFilterToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, "Sepia selection");
    });

    menu.addSeparator();
    menu.addAction(TR("Rogner selon la sélection"), this, &MainWindow::cropToSelection);
    menu.addAction(TR("Inverser la sélection"), this, &MainWindow::invertSelection);
    menu.addAction(TR("Désélectionner"), this, &MainWindow::deselectAll);
    menu.exec(globalPos);
}

std::vector<int> MainWindow::targetLayerIndices() const {
    std::vector<int> indices;
    if (!m_document) return indices;

    if (m_document->editAllLayers()) {
        indices.reserve(m_document->layerCount());
        for (int i = 0; i < m_document->layerCount(); ++i) {
            indices.push_back(i);
        }
        return indices;
    }

    indices.push_back(m_document->activeLayerIndex());
    return indices;
}

void MainWindow::rotateClockwise() {
    if (!m_document) return;
    m_document->rotate(1, "Rotate 90° CW");
    m_canvas->updateCanvas();
}

void MainWindow::rotateCounterClockwise() {
    if (!m_document) return;
    m_document->rotate(3, "Rotate 90° CCW");
    m_canvas->updateCanvas();
}

void MainWindow::rotate180() {
    if (!m_document) return;
    m_document->rotate(2, "Rotate 180°");
    m_canvas->updateCanvas();
}

void MainWindow::flipHorizontal() {
    if (!m_document) return;
    m_document->flip(true, "Flip Horizontal");
    m_canvas->updateCanvas();
}

void MainWindow::flipVertical() {
    if (!m_document) return;
    m_document->flip(false, "Flip Vertical");
    m_canvas->updateCanvas();
}

void MainWindow::cropToSelection() {
    if (!m_document->selection().hasSelection()) return;
    QRect rect = m_document->selection().boundingRect();
    if (rect.isEmpty()) return;

    // Undoable crop that also updates the document dimensions (keeps canvas and
    // document size in sync).
    m_document->cropTo(rect);
    emit m_document->selectionChanged();
    m_canvas->updateCanvas();
    m_sizeLabel->setText(QString("%1 × %2").arg(m_document->width()).arg(m_document->height()));
}

void MainWindow::flattenImage() {
    m_document->flattenImage();
    m_canvas->updateCanvas();
}

// ---- View operations ----

void MainWindow::zoomIn() { m_canvas->zoomIn(); }
void MainWindow::zoomOut() { m_canvas->zoomOut(); }
void MainWindow::zoomToFit() { m_canvas->zoomToFit(); }
void MainWindow::zoomToSelection() {
    if (m_document && m_document->selection().hasSelection())
        m_canvas->zoomToRect(m_document->selection().boundingRect());
    else
        m_canvas->zoomToFit();
}
void MainWindow::zoomToActual() { m_canvas->zoomToActual(); }
void MainWindow::toggleGrid() { m_canvas->setShowGrid(!m_canvas->showGrid()); }

// ---- Tool selection ----

void MainWindow::selectTool(ToolType type) {
    for (auto &tool : m_tools) {
        if (tool->type() == type) {
            m_currentTool = tool.get();
            m_canvas->setCurrentTool(m_currentTool);
            m_canvas->setCursor(m_currentTool->cursor());
            m_toolOptionsPanel->setTool(m_currentTool);
            if (m_toolLabel) m_toolLabel->setText(m_currentTool->name());
            // Keep the tool palette highlight in sync when selection comes from
            // a keyboard shortcut or tool-group cycling (not just a button click).
            if (auto *action = m_toolActions.value(type, nullptr))
                action->setChecked(true);

            // Update help text based on tool type
            if (m_helpTextLabel) {
                switch (type) {
                case ToolType::Brush:
                    m_helpTextLabel->setText(TR("Clic gauche pour dessiner avec la couleur primaire, clic droit avec la couleur secondaire."));
                    break;
                case ToolType::Eraser:
                    m_helpTextLabel->setText(TR("Clic gauche pour effacer vers transparent. Clic droit vers la couleur secondaire."));
                    break;
                case ToolType::Fill:
                    m_helpTextLabel->setText(TR("Clic gauche pour remplir une zone avec la couleur primaire."));
                    break;
                case ToolType::ColorPicker:
                    m_helpTextLabel->setText(TR("Clic gauche pour définir la couleur primaire. Clic droit pour la secondaire."));
                    break;
                case ToolType::RectSelection:
                case ToolType::EllipseSelection:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour tracer une sélection. Maintenir Maj pour contraindre."));
                    break;
                case ToolType::MagicWand:
                    m_helpTextLabel->setText(TR("Cliquer pour sélectionner une zone de couleur similaire."));
                    break;
                case ToolType::Move:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour déplacer les pixels sélectionnés ou le calque."));
                    break;
                case ToolType::MoveSelection:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour déplacer le contour de sélection sans déplacer les pixels."));
                    break;
                case ToolType::Zoom:
                    m_helpTextLabel->setText(TR("Clic gauche pour zoomer. Clic droit pour dézoomer."));
                    break;
                case ToolType::Pan:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour se déplacer dans l'image."));
                    break;
                case ToolType::LassoSelection:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour tracer une sélection à main levée."));
                    break;
                case ToolType::Text:
                    m_helpTextLabel->setText(TR("Cliquer pour placer du texte sur le canevas."));
                    break;
                case ToolType::Line:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour tracer une ligne. Maintenir Maj pour contraindre l'angle."));
                    break;
                case ToolType::Shape:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour tracer une forme. Maintenir Maj pour contraindre les proportions."));
                    break;
                case ToolType::Gradient:
                    m_helpTextLabel->setText(TR("Cliquer-glisser pour tracer un dégradé de couleur."));
                    break;
                case ToolType::CloneStamp:
                    m_helpTextLabel->setText(TR("Ctrl + clic pour définir la source. Clic gauche pour peindre depuis la source."));
                    break;
                case ToolType::Pencil:
                    m_helpTextLabel->setText(TR("Clic gauche pour dessiner en 1px avec la couleur primaire. Clic droit pour la secondaire."));
                    break;
                case ToolType::Recolor:
                    m_helpTextLabel->setText(TR("Clic gauche pour remplacer la couleur secondaire par la primaire."));
                    break;
                default:
                    m_helpTextLabel->setText(TR("Prêt."));
                    break;
                }
            }
            return;
        }
    }
}

// ---- Effects ----

void MainWindow::applyEffect(int effectIndex) {
    auto *layer = m_document->activeLayer();
    if (!layer) return;

    std::unique_ptr<Effect> effect;
    switch (effectIndex) {
    case 0: {
        BlurEffect def;
        PreviewDialog dlg(TR("Flou gaussien"), layer->image(),
            {{TR("Rayon"), 1, 100, def.radius(), ""}},
            [](const QImage &src, const QVector<int> &v) { BlurEffect t; t.setRadius(v[0]); return t.apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        auto *e = new BlurEffect; e->setRadius(dlg.values()[0]);
        effect.reset(e);
        break;
    }
    case 1: {
        SharpenEffect def;
        PreviewDialog dlg(TR("Netteté"), layer->image(),
            {{TR("Quantité"), 1, 100, def.amount(), ""}},
            [](const QImage &src, const QVector<int> &v) { SharpenEffect t; t.setAmount(v[0]); return t.apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        auto *e = new SharpenEffect; e->setAmount(dlg.values()[0]);
        effect.reset(e);
        break;
    }
    case 2: {
        auto *e = new NoiseEffect;
        bool ok;
        int i = QInputDialog::getInt(this, TR("Ajouter du bruit"), TR("Intensité :"), e->intensity(), 1, 100, 1, &ok);
        if (!ok) return;
        e->setIntensity(i);
        effect.reset(e);
        break;
    }
    case 3: {
        effect = std::make_unique<EmbossEffect>();
        break;
    }
    case 4: {
        effect = std::make_unique<EdgeDetectEffect>();
        break;
    }
    case 5: {
        auto *e = new OilPaintEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Peinture à l'huile"), TR("Taille du pinceau :"), e->radius(), 1, 10, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        effect.reset(e);
        break;
    }
    case 6: {
        PixelateEffect def;
        PreviewDialog dlg(TR("Pixéliser"), layer->image(),
            {{TR("Taille de cellule"), 2, 100, def.cellSize(), ""}},
            [](const QImage &src, const QVector<int> &v) { PixelateEffect t; t.setCellSize(v[0]); return t.apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        auto *e = new PixelateEffect; e->setCellSize(dlg.values()[0]);
        effect.reset(e);
        break;
    }
    case 7: {
        auto *e = new MotionBlurEffect;
        bool ok;
        int d = QInputDialog::getInt(this, TR("Flou directionnel"), TR("Distance :"), e->distance(), 1, 100, 1, &ok);
        if (!ok) return;
        e->setDistance(d);
        effect.reset(e);
        break;
    }
    case 8: {
        // Ink Sketch - edge detection + invert for sketch look
        bool ok;
        int strength = QInputDialog::getInt(this, TR("Croquis à l'encre"), TR("Contour encre :"), 50, 1, 100, 1, &ok);
        if (!ok) return;
        applyImageOperationToTargetLayers([strength](const QImage &image) {
            QImage img = image.convertToFormat(QImage::Format_ARGB32);
            EdgeDetectEffect edgeEff;
            QImage edges = edgeEff.apply(img);
            for (int y = 0; y < edges.height(); ++y) {
                QRgb *line = reinterpret_cast<QRgb*>(edges.scanLine(y));
                for (int x = 0; x < edges.width(); ++x) {
                    int r = 255 - qRed(line[x]);
                    int g = 255 - qGreen(line[x]);
                    int b = 255 - qBlue(line[x]);
                    float f = strength / 100.0f;
                    r = qBound(0, int(255 * (1 - f) + r * f), 255);
                    g = qBound(0, int(255 * (1 - f) + g * f), 255);
                    b = qBound(0, int(255 * (1 - f) + b * f), 255);
                    line[x] = qRgba(r, g, b, qAlpha(line[x]));
                }
            }
            return edges;
        }, "Ink Sketch");
        return;
    }
    case 9: {
        bool ok;
        int pencilSize = QInputDialog::getInt(this, TR("Croquis au crayon"), TR("Taille de mine :"), 2, 1, 20, 1, &ok);
        if (!ok) return;
        applyImageOperationToTargetLayers([pencilSize](const QImage &image) {
            QImage img = image.convertToFormat(QImage::Format_ARGB32);
            EdgeDetectEffect edgeEff;
            QImage edges = edgeEff.apply(img);
            for (int y = 0; y < edges.height(); ++y) {
                QRgb *line = reinterpret_cast<QRgb*>(edges.scanLine(y));
                for (int x = 0; x < edges.width(); ++x) {
                    int gray = 255 - qGray(line[x]);
                    line[x] = qRgba(gray, gray, gray, qAlpha(line[x]));
                }
            }
            // Lead size thickens the strokes: a separable min-filter (darkening
            // dilation) of radius pencilSize/2 over the grey lines.
            const int r = pencilSize / 2;
            if (r >= 1) {
                const int w = edges.width(), h = edges.height();
                QImage tmp = edges;
                for (int y = 0; y < h; ++y) {
                    const QRgb *s = reinterpret_cast<const QRgb*>(edges.constScanLine(y));
                    QRgb *d = reinterpret_cast<QRgb*>(tmp.scanLine(y));
                    for (int x = 0; x < w; ++x) {
                        int mn = 255;
                        for (int dx = -r; dx <= r; ++dx) {
                            int nx = x + dx;
                            if (nx < 0 || nx >= w) continue;
                            mn = std::min(mn, qRed(s[nx]));
                        }
                        d[x] = qRgba(mn, mn, mn, qAlpha(s[x]));
                    }
                }
                for (int x = 0; x < w; ++x) {
                    for (int y = 0; y < h; ++y) {
                        int mn = 255;
                        for (int dy = -r; dy <= r; ++dy) {
                            int ny = y + dy;
                            if (ny < 0 || ny >= h) continue;
                            mn = std::min(mn, qRed(reinterpret_cast<const QRgb*>(tmp.constScanLine(ny))[x]));
                        }
                        QRgb cur = reinterpret_cast<const QRgb*>(tmp.constScanLine(y))[x];
                        reinterpret_cast<QRgb*>(edges.scanLine(y))[x] = qRgba(mn, mn, mn, qAlpha(cur));
                    }
                }
            }
            return edges;
        }, "Pencil Sketch");
        return;
    }
    case 10: {
        // Clouds - Perlin-like noise render
        QImage img(m_document->width(), m_document->height(), QImage::Format_ARGB32);
        bool ok;
        int scale = QInputDialog::getInt(this, TR("Nuages"), TR("Échelle :"), 100, 10, 500, 10, &ok);
        if (!ok) return;
        srand(42);
        for (int y = 0; y < img.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                double freq = scale / 100.0;
                double val = sin(x * freq * 0.01) * cos(y * freq * 0.01) * 0.5 + 0.5;
                val += sin(x * freq * 0.02 + y * freq * 0.03) * 0.25;
                val = qBound(0.0, val * 0.7 + 0.15, 1.0);
                int c = static_cast<int>(val * 255);
                line[x] = qRgba(c, c, c, 255);
            }
        }
        applyImageOperationToTargetLayers([img](const QImage &) {
            return img;
        }, "Clouds");
        return;
    }
    case 11: {
        // Julia Fractal
        QImage img(m_document->width(), m_document->height(), QImage::Format_ARGB32);
        int maxIter = 256;
        double cx = -0.7, cy = 0.27015;
        for (int y = 0; y < img.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                double zx = 3.0 * (x - img.width() / 2.0) / img.width();
                double zy = 2.0 * (y - img.height() / 2.0) / img.height();
                int iter = 0;
                while (zx * zx + zy * zy < 4.0 && iter < maxIter) {
                    double tmp = zx * zx - zy * zy + cx;
                    zy = 2.0 * zx * zy + cy;
                    zx = tmp;
                    iter++;
                }
                int c = (iter == maxIter) ? 0 : (iter * 255 / maxIter);
                line[x] = qRgba(c, c / 2, c * 2 > 255 ? 255 : c * 2, 255);
            }
        }
        applyImageOperationToTargetLayers([img](const QImage &) {
            return img;
        }, "Julia Fractal");
        return;
    }
    case 12: {
        // Mandelbrot Fractal
        QImage img(m_document->width(), m_document->height(), QImage::Format_ARGB32);
        int maxIter = 256;
        for (int y = 0; y < img.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                double cx2 = 3.5 * x / img.width() - 2.5;
                double cy2 = 2.0 * y / img.height() - 1.0;
                double zx = 0, zy = 0;
                int iter = 0;
                while (zx * zx + zy * zy < 4.0 && iter < maxIter) {
                    double tmp = zx * zx - zy * zy + cx2;
                    zy = 2.0 * zx * zy + cy2;
                    zx = tmp;
                    iter++;
                }
                int c = (iter == maxIter) ? 0 : (iter * 255 / maxIter);
                line[x] = qRgba(c, c * 3 / 4, c / 2, 255);
            }
        }
        applyImageOperationToTargetLayers([img](const QImage &) {
            return img;
        }, "Mandelbrot Fractal");
        return;
    }
    case 13: {
        auto *e = new RadialBlurEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Flou radial"), TR("Angle :"), e->angle(), 1, 90, 1, &ok);
        if (!ok) return;
        e->setAngle(a);
        e->setCenter(QPoint(layer->image().width() / 2, layer->image().height() / 2));
        effect.reset(e);
        break;
    }
    case 14: {
        auto *e = new ZoomBlurEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Flou de zoom"), TR("Quantité :"), e->amount(), 1, 100, 1, &ok);
        if (!ok) return;
        e->setAmount(a);
        effect.reset(e);
        break;
    }
    case 15: {
        auto *e = new SurfaceBlurEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Flou de surface"), TR("Rayon :"), e->radius(), 1, 20, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        int t = QInputDialog::getInt(this, TR("Flou de surface"), TR("Seuil :"), e->threshold(), 1, 100, 1, &ok);
        if (!ok) return;
        e->setThreshold(t);
        effect.reset(e);
        break;
    }
    case 16: {
        auto *e = new UnfocusEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Flou"), TR("Rayon :"), e->radius(), 1, 50, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        effect.reset(e);
        break;
    }
    case 17: {
        auto *e = new FragmentEffect;
        bool ok;
        int f = QInputDialog::getInt(this, TR("Fragment"), TR("Fragments :"), e->fragments(), 2, 16, 1, &ok);
        if (!ok) return;
        e->setFragments(f);
        int d = QInputDialog::getInt(this, TR("Fragment"), TR("Distance :"), e->distance(), 1, 50, 1, &ok);
        if (!ok) return;
        e->setDistance(d);
        effect.reset(e);
        break;
    }
    case 18: {
        auto *e = new BulgeEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Bombement"), TR("Quantité (-100 à 100) :"), e->amount(), -100, 100, 1, &ok);
        if (!ok) return;
        e->setAmount(a);
        effect.reset(e);
        break;
    }
    case 19: {
        auto *e = new FrostedGlassEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Verre givré"), TR("Quantité :"), e->amount(), 1, 20, 1, &ok);
        if (!ok) return;
        e->setAmount(a);
        effect.reset(e);
        break;
    }
    case 20: {
        auto *e = new CrystalizeEffect;
        bool ok;
        int s = QInputDialog::getInt(this, TR("Cristalliser"), TR("Taille de cellule :"), e->cellSize(), 2, 100, 1, &ok);
        if (!ok) return;
        e->setCellSize(s);
        effect.reset(e);
        break;
    }
    case 21: {
        auto *e = new TileEffect;
        bool ok;
        int s = QInputDialog::getInt(this, TR("Réflexion en mosaïque"), TR("Taille de tuile :"), e->tileSize(), 2, 200, 1, &ok);
        if (!ok) return;
        e->setTileSize(s);
        int r = QInputDialog::getInt(this, TR("Réflexion en mosaïque"), TR("Rotation :"), e->rotation(), -180, 180, 1, &ok);
        if (!ok) return;
        e->setRotation(r);
        effect.reset(e);
        break;
    }
    case 22: {
        auto *e = new DentsEffect;
        bool ok;
        int s = QInputDialog::getInt(this, TR("Bosselure"), TR("Échelle :"), e->scale(), 1, 200, 1, &ok);
        if (!ok) return;
        e->setScale(s);
        int r = QInputDialog::getInt(this, TR("Bosselure"), TR("Réfraction :"), e->refraction(), 1, 200, 1, &ok);
        if (!ok) return;
        e->setRefraction(r);
        effect.reset(e);
        break;
    }
    case 23: {
        auto *e = new PolarInversionEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Inversion polaire"), TR("Quantité :"), e->amount(), 0, 100, 1, &ok);
        if (!ok) return;
        e->setAmount(a);
        effect.reset(e);
        break;
    }
    case 24: {
        auto *e = new TwistEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Torsion"), TR("Quantité (-360 à 360) :"), e->amount(), -360, 360, 1, &ok);
        if (!ok) return;
        e->setAmount(a);
        effect.reset(e);
        break;
    }
    case 25: {
        auto *e = new MedianEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Médiane"), TR("Rayon :"), e->radius(), 1, 10, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        effect.reset(e);
        break;
    }
    case 26: {
        auto *e = new ReduceNoiseEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Réduire le bruit"), TR("Rayon :"), e->radius(), 1, 10, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        int s = QInputDialog::getInt(this, TR("Réduire le bruit"), TR("Force :"), e->strength(), 1, 100, 1, &ok);
        if (!ok) return;
        e->setStrength(s);
        effect.reset(e);
        break;
    }
    case 27: {
        auto *e = new QuantizeEffect;
        bool ok;
        int c = QInputDialog::getInt(this, TR("Quantifier"), TR("Couleurs :"), e->colors(), 2, 256, 1, &ok);
        if (!ok) return;
        e->setColors(c);
        effect.reset(e);
        break;
    }
    case 28: {
        auto *e = new GlowEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Lueur"), TR("Rayon :"), e->radius(), 1, 20, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        int b = QInputDialog::getInt(this, TR("Lueur"), TR("Luminosité :"), e->brightness(), 0, 100, 1, &ok);
        if (!ok) return;
        e->setBrightness(b);
        effect.reset(e);
        break;
    }
    case 29: {
        auto *e = new RedEyeRemoveEffect;
        bool ok;
        int t = QInputDialog::getInt(this, TR("Suppression yeux rouges"), TR("Tolérance :"), e->tolerance(), 0, 100, 1, &ok);
        if (!ok) return;
        e->setTolerance(t);
        int s = QInputDialog::getInt(this, TR("Suppression yeux rouges"), TR("Saturation :"), e->saturation(), 0, 100, 1, &ok);
        if (!ok) return;
        e->setSaturation(s);
        effect.reset(e);
        break;
    }
    case 30: {
        auto *e = new SoftenPortraitEffect;
        bool ok;
        int s = QInputDialog::getInt(this, TR("Adoucir le portrait"), TR("Douceur :"), e->softness(), 1, 20, 1, &ok);
        if (!ok) return;
        e->setSoftness(s);
        int w = QInputDialog::getInt(this, TR("Adoucir le portrait"), TR("Chaleur :"), e->warmth(), 0, 100, 1, &ok);
        if (!ok) return;
        e->setWarmth(w);
        effect.reset(e);
        break;
    }
    case 31: {
        auto *e = new VignetteEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Vignette"), TR("Quantité :"), e->amount(), 0, 100, 1, &ok);
        if (!ok) return;
        e->setAmount(a);
        int r = QInputDialog::getInt(this, TR("Vignette"), TR("Rayon :"), e->radius(), 0, 100, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        effect.reset(e);
        break;
    }
    case 32: {
        auto *e = new TurbulenceEffect;
        bool ok;
        int s = QInputDialog::getInt(this, TR("Turbulence"), TR("Échelle :"), e->scale(), 10, 500, 10, &ok);
        if (!ok) return;
        e->setScale(s);
        int r = QInputDialog::getInt(this, TR("Turbulence"), TR("Rugosité :"), e->roughness(), 1, 100, 1, &ok);
        if (!ok) return;
        e->setRoughness(r);
        effect.reset(e);
        break;
    }
    case 33: {
        auto *e = new ReliefEffect;
        bool ok;
        int a = QInputDialog::getInt(this, TR("Relief"), TR("Angle :"), e->angle(), 0, 360, 1, &ok);
        if (!ok) return;
        e->setAngle(a);
        effect.reset(e);
        break;
    }
    case 34: {
        auto *e = new OutlineEffect;
        bool ok;
        int t = QInputDialog::getInt(this, TR("Contour"), TR("Épaisseur :"), e->thickness(), 1, 10, 1, &ok);
        if (!ok) return;
        e->setThickness(t);
        int i = QInputDialog::getInt(this, TR("Contour"), TR("Intensité :"), e->intensity(), 1, 100, 1, &ok);
        if (!ok) return;
        e->setIntensity(i);
        effect.reset(e);
        break;
    }
    case 35: {
        auto *e = new MorphologyEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Morphologie"), TR("Rayon :"), e->radius(), 1, 10, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        QStringList modes = {"Dilater", "Éroder"};
        QString mode = QInputDialog::getItem(this, TR("Morphologie"), TR("Mode :"), modes, 0, false, &ok);
        if (!ok) return;
        e->setDilate(mode == "Dilater");
        effect.reset(e);
        break;
    }
    case 36: {   // Drop Shadow
        auto *e = new DropShadowEffect;
        bool ok;
        int r = QInputDialog::getInt(this, TR("Ombre portée"), TR("Rayon du flou :"), 8, 0, 50, 1, &ok);
        if (!ok) return;
        e->setRadius(r);
        int ox = QInputDialog::getInt(this, TR("Ombre portée"), TR("Décalage X :"), 6, -100, 100, 1, &ok);
        if (!ok) return;
        int oy = QInputDialog::getInt(this, TR("Ombre portée"), TR("Décalage Y :"), 6, -100, 100, 1, &ok);
        if (!ok) return;
        int op = QInputDialog::getInt(this, TR("Ombre portée"), TR("Opacité (0-100) :"), 70, 0, 100, 1, &ok);
        if (!ok) return;
        e->setOffsetX(ox); e->setOffsetY(oy); e->setOpacity(op);
        effect.reset(e);
        break;
    }
    default: return;
    }

    if (effect) {
        applyImageOperationToTargetLayers([effectPtr = effect.get()](const QImage &image) {
            return effectPtr->apply(image);
        }, effect->name());
    }
}

// ---- Adjustments ----

void MainWindow::applyAdjustment(int adjustmentIndex) {
    auto *layer = m_document->activeLayer();
    if (!layer) return;

    switch (adjustmentIndex) {
    case 0: {
        auto build = [](const QVector<int> &v) {
            BrightnessContrast a; a.setBrightness(v[0]); a.setContrast(v[1]); return a;
        };
        PreviewDialog dlg(TR("Luminosité / Contraste"), layer->image(),
            {{TR("Luminosité"), -100, 100, 0, ""}, {TR("Contraste"), -100, 100, 0, ""}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        BrightnessContrast adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 1: {
        HueSaturationDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;
        HueSaturation adj;
        adj.setHue(dlg.hue());
        adj.setSaturation(dlg.saturation());
        adj.setLightness(dlg.lightness());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 2: {
        // Gamma slider is in percent (100 = 1.0) so it fits the integer sliders.
        auto build = [](const QVector<int> &v) {
            Levels a; a.setInputBlack(v[0]); a.setInputWhite(v[1]); a.setGamma(v[2] / 100.0); return a;
        };
        PreviewDialog dlg(TR("Niveaux"), layer->image(),
            {{TR("Noir d'entrée"), 0, 255, 0, ""},
             {TR("Blanc d'entrée"), 0, 255, 255, ""},
             {TR("Gamma"), 10, 1000, 100, "%"}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        Levels adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 3: {
        CurvesDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;
        Curves adj;
        adj.setControlPoints(dlg.controlPoints());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 4: {
        InvertColors adj;
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 5: {
        auto build = [](const QVector<int> &v) { Sepia a; a.setIntensity(v[0]); return a; };
        PreviewDialog dlg(TR("Sépia"), layer->image(),
            {{TR("Intensité"), 0, 100, 80, "%"}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        Sepia adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 6: {
        auto build = [](const QVector<int> &v) { Posterize a; a.setLevels(v[0]); return a; };
        PreviewDialog dlg(TR("Postériser"), layer->image(),
            {{TR("Niveaux"), 2, 64, 4, ""}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        Posterize adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 7: {
        // Paint.NET "Black and White" desaturates (instant, no threshold prompt).
        Desaturate adj;
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 8: {
        auto build = [](const QVector<int> &v) {
            ColorBalance a; a.setCyanRed(v[0]); a.setMagentaGreen(v[1]); a.setYellowBlue(v[2]); return a;
        };
        PreviewDialog dlg(TR("Balance des couleurs"), layer->image(),
            {{TR("Cyan / Rouge"), -100, 100, 0, ""},
             {TR("Magenta / Vert"), -100, 100, 0, ""},
             {TR("Jaune / Bleu"), -100, 100, 0, ""}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        ColorBalance adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable {
            return adj.apply(image);
        }, adj.name());
        break;
    }
    case 9: {
        // Auto-Level: stretch histogram to full 0-255 range per channel
        applyImageOperationToTargetLayers([](const QImage &image) {
            QImage img = image.convertToFormat(QImage::Format_ARGB32);
            int minR = 255, minG = 255, minB = 255;
            int maxR = 0, maxG = 0, maxB = 0;
            for (int y = 0; y < img.height(); ++y) {
                const QRgb *line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
                for (int x = 0; x < img.width(); ++x) {
                    int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
                    minR = std::min(minR, r); maxR = std::max(maxR, r);
                    minG = std::min(minG, g); maxG = std::max(maxG, g);
                    minB = std::min(minB, b); maxB = std::max(maxB, b);
                }
            }
            auto stretch = [](int val, int lo, int hi) -> int {
                if (hi == lo) return val;
                return qBound(0, (val - lo) * 255 / (hi - lo), 255);
            };
            for (int y = 0; y < img.height(); ++y) {
                QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
                for (int x = 0; x < img.width(); ++x) {
                    line[x] = qRgba(
                        stretch(qRed(line[x]), minR, maxR),
                        stretch(qGreen(line[x]), minG, maxG),
                        stretch(qBlue(line[x]), minB, maxB),
                        qAlpha(line[x]));
                }
            }
            return img;
        }, "Auto-Level");
        break;
    }
    case 10: {   // Exposure
        auto build = [](const QVector<int> &v) { Exposure a; a.setExposure(v[0]); return a; };
        PreviewDialog dlg(TR("Exposition"), layer->image(),
            {{TR("Exposition"), -100, 100, 0, ""}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        Exposure adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable { return adj.apply(image); }, adj.name());
        break;
    }
    case 11: {   // Highlights / Shadows
        auto build = [](const QVector<int> &v) {
            HighlightsShadows a; a.setHighlights(v[0]); a.setShadows(v[1]); return a;
        };
        PreviewDialog dlg(TR("Hautes / Basses lumières"), layer->image(),
            {{TR("Hautes lumières"), -100, 100, 0, ""}, {TR("Basses lumières"), -100, 100, 0, ""}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        HighlightsShadows adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable { return adj.apply(image); }, adj.name());
        break;
    }
    case 12: {   // Temperature / Tint
        auto build = [](const QVector<int> &v) {
            TemperatureTint a; a.setTemperature(v[0]); a.setTint(v[1]); return a;
        };
        PreviewDialog dlg(TR("Température / Teinte"), layer->image(),
            {{TR("Température (froid → chaud)"), -100, 100, 0, ""},
             {TR("Teinte (vert → magenta)"), -100, 100, 0, ""}},
            [build](const QImage &src, const QVector<int> &v) mutable { return build(v).apply(src); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
        TemperatureTint adj = build(dlg.values());
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable { return adj.apply(image); }, adj.name());
        break;
    }
    case 13: {   // Invert Alpha
        InvertAlpha adj;
        applyImageOperationToTargetLayers([adj](const QImage &image) mutable { return adj.apply(image); }, adj.name());
        break;
    }
    }
}

// ---- UI updates ----

void MainWindow::updateTitle() {
    QString filename;
    if (m_document) {
        if (!m_document->filePath().isEmpty()) {
            QFileInfo fi(m_document->filePath());
            filename = fi.fileName();
        } else {
            filename = TR("Sans titre");
        }
        if (m_document->isModified()) filename += " *";
    } else {
        filename = TR("Sans titre");
    }
    setWindowTitle(filename + " - paint.software 1.1");
}

void MainWindow::updateImageThumbnail() {
    if (!m_imageThumbnail || !m_document) return;
    QImage composite(m_document->width(), m_document->height(), QImage::Format_ARGB32);
    composite.fill(Qt::white);
    QPainter p(&composite);
    // flattenVisible() honours each layer's opacity and blend mode (a plain
    // per-layer drawImage ignored both).
    p.drawImage(0, 0, m_document->flattenVisible());
    p.end();
    QPixmap thumb = QPixmap::fromImage(composite.scaled(60, 45, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_imageThumbnail->setPixmap(thumb);
}

void MainWindow::updateStatusBar(const QPoint &pos) {
    m_positionLabel->setText(QString("%1, %2").arg(pos.x()).arg(pos.y()));
}

void MainWindow::onZoomChanged(double zoom) {
    m_zoomLabel->setText(QString("%1%").arg(static_cast<int>(zoom * 100)));
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        saveUiState();
        clearAutosave();   // clean exit -> discard the recovery snapshot
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange)
        syncUtilityWindowsToMainState();
    QMainWindow::changeEvent(event);
}

// Floating utility windows are separate top-level windows, so on some window
// managers they don't follow the main window when it is minimised. Keep them in
// sync: hide the visible floating docks when minimised, restore them when the
// app comes back. Called from changeEvent AND a low-frequency timer, because a
// few WMs (e.g. Muffin/Cinnamon) don't deliver a WindowStateChange on external
// minimise.
void MainWindow::syncUtilityWindowsToMainState() {
    // Only Minimized is authoritative. Minimising under Muffin/Cinnamon walks the
    // window through Hidden -> Windowed -> Minimized, so counting the transient
    // Hidden as minimised hid the panels early, and the Windowed blip that follows
    // read as "restored" and showed them again mid-minimise.
    bool minimized = isMinimized();
    if (QWindow *wh = windowHandle())
        if (wh->visibility() == QWindow::Minimized) minimized = true;
    if (minimized == m_minimized) return;
    m_minimized = minimized;

    if (minimized) {
        m_hiddenOnMinimize.clear();
        m_hiddenOnMinimizeGeometry.clear();
        for (QDockWidget *dock : {m_toolsDock, m_historyDock, m_layersDock, m_colorsDock}) {
            if (!dock || !dock->isVisible()) continue;
            // Hide the panel's top-level window, not the dock: isFloating() is
            // false for a panel Qt has wrapped in a QDockWidgetGroupWindow, and
            // that group window — not the dock — is what stays on screen.
            QWidget *top = dock->window();
            if (!top || top == this || m_hiddenOnMinimize.contains(top)) continue;
            m_hiddenOnMinimize.append(top);
            m_hiddenOnMinimizeGeometry.append(top->geometry());
            top->hide();
        }
    } else {
        for (int i = 0; i < m_hiddenOnMinimize.size(); ++i) {
            QWidget *top = m_hiddenOnMinimize.at(i);
            if (!top) continue;
            top->show();
            // Showing a hidden floating dock can drop its floating state, which
            // re-docks the panel into the main window. Put it back where it was.
            // (A group window is not a QDockWidget, and keeps its own state.)
            if (auto *dock = qobject_cast<QDockWidget *>(top)) {
                if (!dock->isFloating()) {
                    dock->setFloating(true);
                    dock->setGeometry(m_hiddenOnMinimizeGeometry.at(i));
                }
            }
        }
        m_hiddenOnMinimize.clear();
        m_hiddenOnMinimizeGeometry.clear();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            QString filePath = url.toLocalFile();
            QString suffix = QFileInfo(filePath).suffix().toLower();
            if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || 
                suffix == "bmp" || suffix == "gif" || suffix == "tiff" || suffix == "webp") {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!m_document) {
        QMessageBox::warning(this, TR("Erreur"), TR("Veuillez créer un document avant d'ajouter un calque."));
        return;
    }

    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        const QList<QUrl> urls = mimeData->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            QString suffix = QFileInfo(filePath).suffix().toLower();
            if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || 
                suffix == "bmp" || suffix == "gif" || suffix == "tiff" || suffix == "webp") {
                
                QImage image(filePath);
                if (!image.isNull()) {
                    QString layerName = QFileInfo(filePath).baseName();
                    
                    // Calculer la nouvelle taille requise (jamais réduire)
                    int newWidth = qMax(m_document->width(), image.width());
                    int newHeight = qMax(m_document->height(), image.height());
                    
                    // Si le document doit être agrandi, redimensionner le canvas
                    if (newWidth > m_document->width() || newHeight > m_document->height()) {
                        m_document->resizeCanvas(newWidth, newHeight, 0, 0);
                    }
                    
                    // Ensuite redimensionner l'image si nécessaire
                    QImage layerImage = image;
                    if (image.width() < newWidth || image.height() < newHeight) {
                        QImage resizedImage(newWidth, newHeight, QImage::Format_ARGB32_Premultiplied);
                        resizedImage.fill(Qt::transparent);
                        QPainter p(&resizedImage);
                        p.drawImage(0, 0, image);
                        layerImage = resizedImage;
                    }
                    
                    // Ajouter l'image comme nouveau calque
                    m_document->addLayer(layerImage, layerName);
                    m_canvas->update();
                    m_layersPanel->updateLayerList();
                    m_sizeLabel->setText(QString("%1 × %2").arg(m_document->width()).arg(m_document->height()));
                    updateTitle();
                    event->acceptProposedAction();
                } else {
                    QMessageBox::warning(this, TR("Erreur"), TR("Impossible de charger le fichier image."));
                }
            }
        }
    }
}
