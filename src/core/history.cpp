#include "history.h"
#include "document.h"
#include "layer.h"
#include <algorithm>

// ImageEditCommand
ImageEditCommand::ImageEditCommand(Document *doc, int layerIndex, const QImage &before, const QImage &after, const QString &desc)
    : m_doc(doc), m_layerIndex(layerIndex), m_before(before), m_after(after), m_description(desc) {}

void ImageEditCommand::undo() {
    if (m_layerIndex >= 0 && m_layerIndex < m_doc->layerCount()) {
        m_doc->layerAt(m_layerIndex)->setImage(m_before);
        m_doc->notifyContentChanged();
    }
}

void ImageEditCommand::redo() {
    if (m_layerIndex >= 0 && m_layerIndex < m_doc->layerCount()) {
        m_doc->layerAt(m_layerIndex)->setImage(m_after);
        m_doc->notifyContentChanged();
    }
}

// LayerAddCommand
LayerAddCommand::LayerAddCommand(Document *doc, int index, const QString &desc)
    : m_doc(doc), m_index(index), m_description(desc) {}

void LayerAddCommand::undo() {
    m_savedLayer = std::make_shared<Layer>(*m_doc->layerAt(m_index));
    m_doc->removeLayerDirect(m_index);
}

void LayerAddCommand::redo() {
    if (m_savedLayer)
        m_doc->insertLayerDirect(m_index, std::make_shared<Layer>(*m_savedLayer));
}

// LayerRemoveCommand
LayerRemoveCommand::LayerRemoveCommand(Document *doc, int index, const QString &desc)
    : m_doc(doc), m_index(index), m_description(desc) {
    m_savedLayer = std::make_shared<Layer>(*m_doc->layerAt(index));
}

void LayerRemoveCommand::undo() {
    m_doc->insertLayerDirect(m_index, std::make_shared<Layer>(*m_savedLayer));
}

void LayerRemoveCommand::redo() {
    m_doc->removeLayerDirect(m_index);
}

// LayerMoveCommand
LayerMoveCommand::LayerMoveCommand(Document *doc, int fromIndex, int toIndex, const QString &desc)
    : m_doc(doc), m_fromIndex(fromIndex), m_toIndex(toIndex), m_description(desc) {}

void LayerMoveCommand::undo() {
    m_doc->moveLayerDirect(m_toIndex, m_fromIndex);
}

void LayerMoveCommand::redo() {
    m_doc->moveLayerDirect(m_fromIndex, m_toIndex);
}

// DocumentSnapshotCommand
DocumentSnapshotCommand::DocumentSnapshotCommand(Document *doc, const QString &desc)
    : m_doc(doc), m_description(desc) {
    m_before = capture(doc);
}

void DocumentSnapshotCommand::captureAfter() {
    m_after = capture(m_doc);
    m_hasAfter = true;
}

DocumentSnapshotCommand::State DocumentSnapshotCommand::capture(Document *doc) {
    State s;
    s.width = doc->width();
    s.height = doc->height();
    s.activeLayer = doc->activeLayerIndex();
    s.layers = doc->snapshotLayers();
    return s;
}

void DocumentSnapshotCommand::restore(Document *doc, const State &state) {
    std::vector<std::shared_ptr<Layer>> copy;
    copy.reserve(state.layers.size());
    for (const auto &l : state.layers)
        copy.push_back(std::make_shared<Layer>(*l));
    doc->restoreFromSnapshot(std::move(copy), state.width, state.height, state.activeLayer);
}

void DocumentSnapshotCommand::undo() {
    restore(m_doc, m_before);
}

void DocumentSnapshotCommand::redo() {
    if (m_hasAfter)
        restore(m_doc, m_after);
}

// LayerPropertyCommand
LayerPropertyCommand::LayerPropertyCommand(Document *doc, int index,
                                           float beforeOpacity, bool beforeVisible, int beforeBlend,
                                           const QString &beforeName, bool beforeLocked,
                                           const QString &desc)
    : m_doc(doc), m_index(index), m_description(desc) {
    m_before = {beforeOpacity, beforeVisible, beforeBlend, beforeName, beforeLocked};
}

void LayerPropertyCommand::captureAfter() {
    if (auto *l = m_doc->layerAt(m_index)) {
        m_after = {l->opacity(), l->isVisible(), static_cast<int>(l->blendMode()),
                   l->name(), l->isLocked()};
        m_hasAfter = true;
    }
}

void LayerPropertyCommand::apply(const Props &p) {
    if (auto *l = m_doc->layerAt(m_index)) {
        l->setOpacity(p.opacity);
        l->setVisible(p.visible);
        l->setBlendMode(static_cast<BlendMode>(p.blend));
        l->setName(p.name);
        l->setLocked(p.locked);
        m_doc->notifyContentChanged();
    }
}

void LayerPropertyCommand::undo() { apply(m_before); }
void LayerPropertyCommand::redo() { if (m_hasAfter) apply(m_after); }

// HistoryManager
HistoryManager::HistoryManager(QObject *parent) : QObject(parent) {}

void HistoryManager::push(std::unique_ptr<HistoryCommand> cmd) {
    m_redoStack.clear();
    m_undoStack.push_back(std::move(cmd));
    // Cap the undo depth to keep memory bounded (each image edit stores full
    // before/after frames).
    const size_t kMaxUndo = 100;
    if (m_undoStack.size() > kMaxUndo)
        m_undoStack.erase(m_undoStack.begin(),
                          m_undoStack.begin() + (m_undoStack.size() - kMaxUndo));
    emit historyChanged();
}

void HistoryManager::undo() {
    if (m_undoStack.empty()) return;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->undo();
    m_redoStack.push_back(std::move(cmd));
    emit historyChanged();
}

void HistoryManager::redo() {
    if (m_redoStack.empty()) return;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->redo();
    m_undoStack.push_back(std::move(cmd));
    emit historyChanged();
}

void HistoryManager::goToIndex(int index) {
    const int total = static_cast<int>(m_undoStack.size() + m_redoStack.size());
    index = std::max(0, std::min(index, total));
    // currentIndex() == m_undoStack.size(); undo to shrink it, redo to grow it.
    while (static_cast<int>(m_undoStack.size()) > index) undo();
    while (static_cast<int>(m_undoStack.size()) < index) redo();
}

QString HistoryManager::undoDescription() const {
    if (m_undoStack.empty()) return QString();
    return m_undoStack.back()->description();
}

QString HistoryManager::redoDescription() const {
    if (m_redoStack.empty()) return QString();
    return m_redoStack.back()->description();
}

QStringList HistoryManager::historyList() const {
    QStringList list;
    for (const auto &cmd : m_undoStack)
        list.append(cmd->description());
    return list;
}

QStringList HistoryManager::fullHistoryList() const {
    QStringList list;
    for (const auto &cmd : m_undoStack)
        list.append(cmd->description());
    // Redo stack is in reverse chronological order (back() = next to redo).
    for (auto it = m_redoStack.rbegin(); it != m_redoStack.rend(); ++it)
        list.append((*it)->description());
    return list;
}

void HistoryManager::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
    emit historyChanged();
}
