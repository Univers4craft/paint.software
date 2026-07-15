#pragma once

#include <QObject>
#include <QImage>
#include <QColor>
#include <QSize>
#include <memory>
#include <vector>

#include "layer.h"
#include "selection.h"
#include "history.h"

class Document : public QObject {
    Q_OBJECT
public:
    explicit Document(int width = 800, int height = 600, QObject *parent = nullptr);
    explicit Document(const QString &filePath, QObject *parent = nullptr);

    // Size
    int width() const { return m_width; }
    int height() const { return m_height; }
    QSize size() const { return QSize(m_width, m_height); }
    void resize(int width, int height);
    void resizeCanvas(int width, int height, int anchorX = 0, int anchorY = 0);

    // Layers
    int layerCount() const { return static_cast<int>(m_layers.size()); }
    Layer *layerAt(int index);
    const Layer *layerAt(int index) const;
    Layer *activeLayer();
    int activeLayerIndex() const { return m_activeLayer; }
    void setActiveLayer(int index);

    int addLayer(const QString &name = QString());
    int addLayer(const QImage &image, const QString &name = QString());
    void removeLayer(int index);
    void moveLayer(int fromIndex, int toIndex);
    void duplicateLayer(int index);
    void mergeLayerDown(int index, bool keepArtwork = false);
    void flattenImage();

    // Direct layer manipulation (for undo/redo)
    void insertLayerDirect(int index, std::shared_ptr<Layer> layer);
    void removeLayerDirect(int index);
    void moveLayerDirect(int fromIndex, int toIndex);

    // Full-document snapshot support (for undo/redo of structural ops)
    std::vector<std::shared_ptr<Layer>> snapshotLayers() const;
    void restoreFromSnapshot(std::vector<std::shared_ptr<Layer>> layers,
                             int width, int height, int activeLayer);

    // Undoable structural operations
    void cropTo(const QRect &rect);
    // Whole-image geometric transforms: rotate every layer and (for 90°/270°)
    // swap the document dimensions. quarterTurns is clockwise 90° steps.
    void rotate(int quarterTurns, const QString &description);
    void flip(bool horizontal, const QString &description);
    // Reorder the whole stack. newOrder holds current layer indices in their new
    // bottom-to-top position. Undoable. keepActive = the layer to stay active.
    void reorderLayers(const std::vector<int> &newOrder, int keepActiveOriginalIndex);

    // Layer property mutation (emits documentChanged so the canvas repaints)
    void setLayerOpacity(int index, float opacity);
    void setLayerVisibility(int index, bool visible);
    void setLayerBlendMode(int index, BlendMode mode);
    void setLayerName(int index, const QString &name);
    void setLayerLocked(int index, bool locked);

    // Selection
    Selection &selection() { return m_selection; }
    const Selection &selection() const { return m_selection; }

    // History
    HistoryManager &history() { return m_history; }
    void pushImageEdit(int layerIndex, const QImage &before, const QString &desc);
    // Emitted by history commands so the canvas and panels refresh after undo/redo.
    void notifyContentChanged() { emit documentChanged(); emit layersChanged(); }

    // Rendering
    QImage flatten() const;
    QImage flattenVisible() const;

    // File I/O
    bool save(const QString &filePath);
    bool load(const QString &filePath);
    // Native layered format (.psw) — preserves every layer, not just a flattened
    // image. Used automatically when the path ends in ".psw".
    bool saveNative(const QString &filePath);
    bool loadNative(const QString &filePath);
    static bool isNativeFormat(const QString &filePath);
    QString filePath() const { return m_filePath; }
    bool isModified() const { return m_modified; }
    void setModified(bool modified) { m_modified = modified; }
    bool editAllLayers() const { return m_editAllLayers; }
    void setEditAllLayers(bool editAllLayers);

    // Colors
    QColor primaryColor() const { return m_primaryColor; }
    QColor secondaryColor() const { return m_secondaryColor; }
    void setPrimaryColor(const QColor &color);
    void setSecondaryColor(const QColor &color);
    void swapColors();

signals:
    void layersChanged();
    void activeLayerChanged(int index);
    void selectionChanged();
    void documentChanged();
    void editScopeChanged(bool editAllLayers);
    void primaryColorChanged(const QColor &color);
    void secondaryColorChanged(const QColor &color);
    void sizeChanged(int width, int height);

private:
    int m_width;
    int m_height;
    std::vector<std::shared_ptr<Layer>> m_layers;
    int m_activeLayer = 0;
    Selection m_selection;
    HistoryManager m_history;
    QString m_filePath;
    bool m_modified = false;
    bool m_editAllLayers = false;
    QColor m_primaryColor = Qt::black;
    QColor m_secondaryColor = Qt::white;
};
