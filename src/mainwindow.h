#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QDockWidget>
#include <QPointer>
#include <QStatusBar>
#include <QLabel>
#include <QAction>
#include <QActionGroup>
#include <QProgressBar>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <functional>
#include <memory>
#include <vector>
#include "tools/tool.h"

class QMenu;
class Document;
class CanvasWidget;
class LayersPanel;
class ColorsPanel;
class HistoryPanel;
class ToolOptionsPanel;
class PluginManager;
class PluginEffect;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Open a file into the image list (used for command-line arguments).
    void openFile(const QString &filePath);

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;   // follow minimize/restore
private:
    void syncUtilityWindowsToMainState();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // File
    void newDocument();
    void openDocument();
    void openRecentFile();
    bool saveDocument();
    bool saveDocumentAs();
    void printDocument();
    void closeCurrentDocument();
    void showSettingsDialog();
    void showShortcutsHelp();

    // Edit
    void undo();
    void redo();
    void cut();
    void copy();
    void copyMerged();
    void paste();
    void pasteIntoNewLayer();
    void pasteIntoNewImage();   // clipboard image -> its own document (Ctrl+Alt+V)
    void fillSelection();       // fill the selection with the primary colour (Backspace)
    void selectAll();
    void deselectAll();
    void growSelection();       // expand the selection outward by N px
    void shrinkSelection();     // contract the selection inward by N px
    void featherSelection();    // soften the selection edge by N px
    void moveActiveLayerUp();
    void moveActiveLayerDown();
    void importLayerFromFile();     // add an image file as a new layer
    void flipActiveLayerHorizontal();
    void flipActiveLayerVertical();
    void invertSelection();

    // Image
    void resizeImage();
    void canvasSize();
    void rotateClockwise();
    void rotateCounterClockwise();
    void rotate180();
    void flipHorizontal();
    void flipVertical();
    void cropToSelection();
    void flattenImage();

    // View
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToSelection();
    void zoomToActual();
    void toggleGrid();

    // Tools
    void selectTool(ToolType type);

    // Effects
    void applyEffect(int effectIndex);
    // Runs an external plugin effect (via the live-preview dialog if it has params).
    void runPluginEffect(PluginEffect *effect);

    // Adjustments
    void applyAdjustment(int adjustmentIndex);

    // UI updates
    void updateTitle();
    void updateStatusBar(const QPoint &pos);
    void onZoomChanged(double zoom);
    void updateImageThumbnail();

private:
    void createMenus();
    // Loads external effect plugins and fills the Effects ▸ Plugins submenu.
    void populatePluginsMenu(QMenu *pluginsMenu);
    void createToolBar();
    void createToolsPanel();
    void createDockPanels();
    void createStatusBar();
    void createTools();
    void createKeyboardShortcuts();
    void applyImageOperation(const QImage &result, const QString &description);
    void applyImageOperationToTargetLayers(const std::function<QImage(const QImage &)> &operation, const QString &description);
    void applySelectionFilterToTargetLayers(const std::function<QImage(const QImage &)> &operation, const QString &description);
    void transformSelectionContents(const std::function<QImage(const QImage &)> &operation, const QString &description);
    void deleteSelectionContents();
    std::vector<int> targetLayerIndices() const;
    QAction *addShortcutAction(const QString &text, const QList<QKeySequence> &shortcuts,
                               const std::function<void()> &handler,
                               Qt::ShortcutContext context = Qt::WindowShortcut);
    bool allowSingleKeyShortcuts() const;
    void cycleToolGroup(const std::vector<ToolType> &types, bool reverse = false);
    void selectLayerAbove();
    void selectLayerBelow();
    void selectTopLayer();
    void selectBottomLayer();
    void adjustCurrentToolBrushSize(int delta);
    void toggleActiveLayerVisibility();
    void showLayerPropertiesDialog();
    void showSelectionContextMenu(const QPoint &globalPos);
    void loadUiState();
    void saveUiState();
    void normalizeDockLayout(QDockWidget *changedDock);
    Qt::Orientation splitOrientationForArea(Qt::DockWidgetArea area) const;
    void importImageAsLayer(const QString &filePath);
    // Returns true if it is safe to discard the current document (saved, or the
    // user chose to discard / had no unsaved changes); false to cancel.
    bool maybeSave();
    bool maybeSaveDocument(Document *doc);
    void loadDocumentInto(const QString &filePath);
    void wireDocumentSignals();
    void addRecentFile(const QString &filePath);
    void updateRecentFilesMenu();
    void createMenuBarCornerIcons();
    void resetUtilityWindow(QDockWidget *dock);
    void populateFixedToolbar();
    void rebuildToolsPaletteTooltips();
    void layoutToolsPalette(bool horizontal);
    // Applies the current colour scheme (light / dark / follow-OS) everywhere.
    void applyTheme();
    // Rebuilds all text-carrying chrome after a language change.
    void retranslateUi();

    // Multi-document support (paint.net keeps several images open at once and
    // shows them as thumbnails in the image list).
    void addDocument(Document *doc);
    void setActiveDocument(int index);
    void closeDocument(int index);
    void updateImageList();

    // Autosave / crash recovery.
    void performAutosave();
    void clearAutosave();
    void checkForRecovery();
    QString autosaveDir() const;

    Document *m_document = nullptr;   // == m_documents[m_activeDocIndex]
    QVector<Document*> m_documents;
    int m_activeDocIndex = -1;
    class ImageListBar *m_imageListBar = nullptr;

    CanvasWidget *m_canvas = nullptr;
    QMenu *m_recentFilesMenu = nullptr;
    QStringList m_recentFiles;
    QString m_lastEffectLabel;
    std::function<void()> m_lastEffect;
    QAction *m_repeatEffectAction = nullptr;
    QAction *m_gridAction = nullptr;
    QAction *m_undoToolbarAction = nullptr;
    QAction *m_redoToolbarAction = nullptr;

    // Tools
    std::vector<std::unique_ptr<Tool>> m_tools;
    Tool *m_currentTool = nullptr;
    QActionGroup *m_toolActionGroup = nullptr;
    QMap<ToolType, QAction*> m_toolActions;
    QVector<class QToolButton*> m_toolButtons;
    class QGridLayout *m_toolsGrid = nullptr;
    QWidget *m_toolsPaletteWidget = nullptr;

    // Panels
    LayersPanel *m_layersPanel = nullptr;
    ColorsPanel *m_colorsPanel = nullptr;
    HistoryPanel *m_historyPanel = nullptr;
    ToolOptionsPanel *m_toolOptionsPanel = nullptr;

    // External effect plugins (loaded once, owned for the app's lifetime).
    std::unique_ptr<PluginManager> m_pluginManager;

    // Dock widgets (for Window menu toggle)
    QDockWidget *m_toolsDock = nullptr;
    QDockWidget *m_historyDock = nullptr;
    QDockWidget *m_layersDock = nullptr;
    QDockWidget *m_colorsDock = nullptr;

    // Top bars
    QToolBar *m_fixedToolbar = nullptr;
    QToolBar *m_variableToolbar = nullptr;

    // Image thumbnail
    QLabel *m_imageThumbnail = nullptr;

    // Status bar
    QLabel *m_helpTextLabel = nullptr;
    QLabel *m_sizeLabel = nullptr;
    QLabel *m_positionLabel = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QLabel *m_toolLabel = nullptr;

    // Actions
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_rulersAction = nullptr;

    bool m_normalizingDockLayout = false;
    bool m_restoringState = false;
    bool m_minimized = false;
    // Top-level windows hidden with the app. Not always the QDockWidget itself:
    // Qt wraps grouped panels in a QDockWidgetGroupWindow, which owns the window.
    QVector<QPointer<QWidget>> m_hiddenOnMinimize;
    QVector<QRect> m_hiddenOnMinimizeGeometry;  // their geometry, to restore on show
};
