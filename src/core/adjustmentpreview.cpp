#include "adjustmentpreview.h"
#include "../canvas/canvaswidget.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>

AdjustmentPreview::AdjustmentPreview(Document *doc, CanvasWidget *canvas,
                                     const std::vector<int> &targets, QObject *parent,
                                     bool useSelection)
    : QObject(parent), m_document(doc), m_canvas(canvas) {
    if (canvas) connect(canvas, &QObject::destroyed, this, &AdjustmentPreview::cancel);
    m_snapshot.size = doc->size();
    if (useSelection) m_snapshot.selection = doc->selection();
    m_snapshot.targets = targets;
    for (int i = 0; i < doc->layerCount(); ++i) m_snapshot.layers.push_back(*doc->layerAt(i));
    m_timer.setSingleShot(true);
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &AdjustmentPreview::start);
    // A modal dialog prevents ordinary edits; invalidate on any external mutation.
    connect(doc, &Document::documentChanged, this, &AdjustmentPreview::cancel);
    connect(doc, &Document::selectionChanged, this, &AdjustmentPreview::cancel);
    connect(doc, &Document::layersChanged, this, &AdjustmentPreview::cancel);
    connect(doc, &QObject::destroyed, this, &AdjustmentPreview::cancel);
}
AdjustmentPreview::~AdjustmentPreview() { cancel(); }
void AdjustmentPreview::cancel() {
    m_active = false;
    m_cancelled->store(true);
    m_pending = {};
    m_timer.stop();
    if (m_canvas) m_canvas->clearPreviewImage();
}
void AdjustmentPreview::request(ImageOperation operation) {
    if (!m_active) return;
    m_pending = std::move(operation);
    if (!m_running && !m_timer.isActive()) m_timer.start();
}
AdjustmentPreview::Result AdjustmentPreview::render(const Snapshot &snapshot,
        const ImageOperation &operation, const std::shared_ptr<std::atomic_bool> &cancelled, bool composite) {
    Result result;
    auto layers = snapshot.layers;
    for (int index : snapshot.targets) {
        if (cancelled->load()) return {};
        if (index < 0 || index >= static_cast<int>(layers.size())) continue;
        auto &layer = layers[index];
        layer.setImage(adjustedImage(layer.image(), layer.offset(), snapshot.selection, operation));
    }
    if (cancelled->load()) return {};
    if (composite) result.composite = Document::compositeLayers(layers, snapshot.size);
    if (cancelled->load()) return {};
    for (const auto &layer : layers) result.images.push_back(layer.image());
    return result;
}
void AdjustmentPreview::start() {
    if (!m_active || m_running || !m_pending) return;
    m_running = true;
    auto operation = std::move(m_pending);
    m_pending = {};
    auto *watcher = new QFutureWatcher<Result>(this);
    connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher]() {
        m_running = false;
        if (m_active && m_canvas && m_canvas->document() != m_document) cancel();
        if (m_active) {
            const auto result = watcher->result();
            if (!result.composite.isNull()) {
                if (m_canvas) m_canvas->setPreviewImage(result.composite);
                emit previewReady(result.composite);
            }
        }
        watcher->deleteLater();
        if (m_active && m_pending) m_timer.start();
    });
    watcher->setFuture(QtConcurrent::run([snapshot = m_snapshot, operation, token = m_cancelled]() {
        return render(snapshot, operation, token);
    }));
}
bool AdjustmentPreview::commit(const ImageOperation &operation, const QString &description) {
    if (!m_active || !m_document || (m_canvas && m_canvas->document() != m_document)) {
        cancel(); return false;
    }
    // Final parameters are authoritative, including OK before the last worker finishes.
    cancel();
    auto result = render(m_snapshot, operation, std::make_shared<std::atomic_bool>(false), false);
    std::vector<ImageEditState> edits;
    for (int i : m_snapshot.targets) {
        if (i < 0 || i >= static_cast<int>(result.images.size())) continue;
        const auto &before = m_snapshot.layers[i].image();
        if (before != result.images[i]) edits.push_back({i, before, result.images[i]});
    }
    return m_document->commitImageEdits(edits, description);
}
