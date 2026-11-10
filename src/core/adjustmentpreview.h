#pragma once
#include "document.h"
#include "imageoperation.h"
#include <QPointer>
#include <QTimer>
#include <atomic>
class CanvasWidget;
// GUI-thread session. Workers receive only value snapshots and a cancellation token.
class AdjustmentPreview : public QObject {
    Q_OBJECT
public:
    // `useSelection` false applies the operation to whole layers even when a
    // selection is active (Layers > Rotate / Zoom works on the full layer).
    AdjustmentPreview(Document *document, CanvasWidget *canvas,
                      const std::vector<int> &targets, QObject *parent = nullptr,
                      bool useSelection = true);
    ~AdjustmentPreview() override;
    // Operations must capture worker-safe values, never live document/UI pointers.
    void request(ImageOperation operation);
    void cancel();
    bool commit(const ImageOperation &operation, const QString &description);
signals:
    void previewReady(const QImage &image);
private:
    struct Snapshot {
        QSize size;
        std::vector<Layer> layers;
        std::vector<int> targets;
        Selection selection;
    };
    struct Result { std::vector<QImage> images; QImage composite; };
    static Result render(const Snapshot &, const ImageOperation &, const std::shared_ptr<std::atomic_bool> &, bool composite = true);
    void start();
    QPointer<Document> m_document;
    QPointer<CanvasWidget> m_canvas;
    Snapshot m_snapshot;
    QTimer m_timer;
    ImageOperation m_pending;
    bool m_running = false;
    bool m_active = true;
    std::shared_ptr<std::atomic_bool> m_cancelled = std::make_shared<std::atomic_bool>(false);
};
