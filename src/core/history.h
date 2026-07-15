#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

class Document;

class HistoryCommand {
public:
    virtual ~HistoryCommand() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual QString description() const = 0;
};

class ImageEditCommand : public HistoryCommand {
public:
    ImageEditCommand(Document *doc, int layerIndex, const QImage &before, const QImage &after, const QString &desc);
    void undo() override;
    void redo() override;
    QString description() const override { return m_description; }
private:
    Document *m_doc;
    int m_layerIndex;
    QImage m_before;
    QImage m_after;
    QString m_description;
};

class LayerAddCommand : public HistoryCommand {
public:
    LayerAddCommand(Document *doc, int index, const QString &desc);
    void undo() override;
    void redo() override;
    QString description() const override { return m_description; }
private:
    Document *m_doc;
    int m_index;
    QString m_description;
    std::shared_ptr<class Layer> m_savedLayer;
};

class LayerRemoveCommand : public HistoryCommand {
public:
    LayerRemoveCommand(Document *doc, int index, const QString &desc);
    void undo() override;
    void redo() override;
    QString description() const override { return m_description; }
private:
    Document *m_doc;
    int m_index;
    QString m_description;
    std::shared_ptr<class Layer> m_savedLayer;
};

class LayerMoveCommand : public HistoryCommand {
public:
    LayerMoveCommand(Document *doc, int fromIndex, int toIndex, const QString &desc);
    void undo() override;
    void redo() override;
    QString description() const override { return m_description; }
private:
    Document *m_doc;
    int m_fromIndex;
    int m_toIndex;
    QString m_description;
};

// Generic full-document snapshot: captures the entire layer stack, size and
// active index. Used for structural operations that can't be represented by a
// single fine-grained command (merge, flatten, crop, resize, canvas size...).
class DocumentSnapshotCommand : public HistoryCommand {
public:
    DocumentSnapshotCommand(Document *doc, const QString &desc);
    // Call after the mutation has been performed to record the "after" state.
    void captureAfter();
    void undo() override;
    void redo() override;
    QString description() const override { return m_description; }
private:
    struct State {
        int width = 0;
        int height = 0;
        int activeLayer = 0;
        std::vector<std::shared_ptr<class Layer>> layers;
    };
    static State capture(Document *doc);
    static void restore(Document *doc, const State &state);
    Document *m_doc;
    QString m_description;
    State m_before;
    State m_after;
    bool m_hasAfter = false;
};

// Records a change to a single layer's properties (opacity, visibility,
// blend mode, name, lock) so those edits become undoable.
class LayerPropertyCommand : public HistoryCommand {
public:
    LayerPropertyCommand(Document *doc, int index,
                         float beforeOpacity, bool beforeVisible, int beforeBlend,
                         const QString &beforeName, bool beforeLocked,
                         const QString &desc);
    void captureAfter();
    void undo() override;
    void redo() override;
    QString description() const override { return m_description; }
private:
    struct Props { float opacity; bool visible; int blend; QString name; bool locked; };
    void apply(const Props &p);
    Document *m_doc;
    int m_index;
    Props m_before;
    Props m_after;
    QString m_description;
    bool m_hasAfter = false;
};

class HistoryManager : public QObject {
    Q_OBJECT
public:
    explicit HistoryManager(QObject *parent = nullptr);

    void push(std::unique_ptr<HistoryCommand> cmd);
    void undo();
    void redo();
    // Jump to a given point in history (0 = original, N = after N actions) by
    // undoing/redoing as needed. Used by the History panel's click-to-navigate.
    void goToIndex(int index);

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    QString undoDescription() const;
    QString redoDescription() const;

    QStringList historyList() const;
    // Full timeline: done actions followed by the still-available redo actions.
    QStringList fullHistoryList() const;
    int currentIndex() const { return static_cast<int>(m_undoStack.size()); }

    void clear();

signals:
    void historyChanged();

private:
    std::vector<std::unique_ptr<HistoryCommand>> m_undoStack;
    std::vector<std::unique_ptr<HistoryCommand>> m_redoStack;
};
