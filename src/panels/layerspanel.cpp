#include "layerspanel.h"
#include "core/document.h"
#include "i18n.h"
#include "theme.h"
#include "core/layer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QLineEdit>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

namespace {
// Neutral "ink" colour and layer-fill colour that adapt to the active theme so
// the icons stay visible on both light and dark panels.
static QColor inkColor()    { return Theme::isDark() ? QColor(210, 210, 210) : QColor(60, 60, 60); }
static QColor layerFill1()  { return Theme::isDark() ? QColor(95, 105, 125)  : QColor(200, 210, 230); }
static QColor layerFill2()  { return Theme::isDark() ? QColor(130, 140, 160) : QColor(230, 235, 245); }
static QColor accentBlue()  { return Theme::isDark() ? QColor(90, 150, 235)  : QColor(50, 100, 200); }

static QIcon makeLayerBtnIcon(const std::function<void(QPainter&)> &drawFunc) {
    QPixmap pm(14, 14);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    drawFunc(p);
    p.end();
    return QIcon(pm);
}
static QIcon addLayerIcon() {
    return makeLayerBtnIcon([](QPainter &p) {
        p.setPen(QPen(QColor(70, 180, 70), 2));   // green reads on both themes
        p.drawLine(7, 2, 7, 12);
        p.drawLine(2, 7, 12, 7);
    });
}
static QIcon removeLayerIcon() {
    return makeLayerBtnIcon([](QPainter &p) {
        p.setPen(QPen(QColor(225, 80, 80), 2));    // red reads on both themes
        p.drawLine(2, 7, 12, 7);
    });
}
static QIcon duplicateLayerIcon() {
    return makeLayerBtnIcon([](QPainter &p) {
        p.setPen(QPen(inkColor(), 1));
        p.setBrush(layerFill1());
        p.drawRect(4, 1, 9, 9);
        p.setBrush(layerFill2());
        p.drawRect(1, 4, 9, 9);
    });
}
static QIcon mergeDownIcon() {
    return makeLayerBtnIcon([](QPainter &p) {
        const QColor ink = inkColor();
        p.setPen(QPen(ink, 1.5));
        p.drawLine(7, 1, 7, 9);
        p.setBrush(ink);
        p.setPen(Qt::NoPen);
        QPointF arr[] = {QPointF(7, 11), QPointF(4, 7), QPointF(10, 7)};
        p.drawPolygon(arr, 3);
        p.setPen(QPen(ink, 1.5));
        p.drawLine(2, 12, 12, 12);
    });
}
static QIcon moveUpIcon() {
    return makeLayerBtnIcon([](QPainter &p) {
        const QColor c = accentBlue();
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        QPointF arr[] = {QPointF(7, 2), QPointF(2, 8), QPointF(12, 8)};
        p.drawPolygon(arr, 3);
        p.setPen(QPen(c, 2));
        p.drawLine(7, 8, 7, 13);
    });
}
static QIcon moveDownIcon() {
    return makeLayerBtnIcon([](QPainter &p) {
        const QColor c = accentBlue();
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        QPointF arr[] = {QPointF(7, 12), QPointF(2, 6), QPointF(12, 6)};
        p.drawPolygon(arr, 3);
        p.setPen(QPen(c, 2));
        p.drawLine(7, 1, 7, 6);
    });
}
} // anon namespace

LayersPanel::LayersPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Debounce timer for live thumbnail refresh (coalesces rapid edits).
    m_thumbRefreshTimer = new QTimer(this);
    m_thumbRefreshTimer->setSingleShot(true);
    m_thumbRefreshTimer->setInterval(80);
    connect(m_thumbRefreshTimer, &QTimer::timeout, this, &LayersPanel::refreshThumbnails);

    // Mode de fusion and opacity are hidden (only in layer properties dialog)
    // But we still create them to keep the signal connections
    m_blendModeCombo = new QComboBox(this);
    for (auto mode : Layer::allBlendModes())
        m_blendModeCombo->addItem(Layer::blendModeName(mode));
    m_blendModeCombo->hide();

    m_opacitySlider = new QSlider(Qt::Horizontal, this);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->hide();

    m_opacityLabel = new QLabel("100%", this);
    m_opacityLabel->setFixedWidth(40);
    m_opacityLabel->hide();

    // Layer list — fills the panel
    m_layerList = new QListWidget;
    m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_layerList->setDragDropMode(QAbstractItemView::InternalMove);
    m_layerList->setIconSize(QSize(50, 36));
    m_layerList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_layerList, &QListWidget::customContextMenuRequested,
            this, &LayersPanel::showLayerContextMenu);
    layout->addWidget(m_layerList);

    // Buttons with proper icons
    auto *btnLayout = new QHBoxLayout;
    m_addBtn = new QPushButton;
    m_addBtn->setIcon(addLayerIcon());
    m_addBtn->setIconSize(QSize(14, 14));
    m_addBtn->setToolTip(TR("Ajouter un calque"));
    m_addBtn->setFixedSize(26, 26);
    m_removeBtn = new QPushButton;
    m_removeBtn->setIcon(removeLayerIcon());
    m_removeBtn->setIconSize(QSize(14, 14));
    m_removeBtn->setToolTip(TR("Supprimer le calque"));
    m_removeBtn->setFixedSize(26, 26);
    m_duplicateBtn = new QPushButton;
    m_duplicateBtn->setIcon(duplicateLayerIcon());
    m_duplicateBtn->setIconSize(QSize(14, 14));
    m_duplicateBtn->setToolTip(TR("Dupliquer le calque"));
    m_duplicateBtn->setFixedSize(26, 26);
    m_mergeBtn = new QPushButton;
    m_mergeBtn->setIcon(mergeDownIcon());
    m_mergeBtn->setIconSize(QSize(14, 14));
    m_mergeBtn->setToolTip(TR("Fusionner vers le bas"));
    m_mergeBtn->setFixedSize(26, 26);
    m_moveUpBtn = new QPushButton;
    m_moveUpBtn->setIcon(moveUpIcon());
    m_moveUpBtn->setIconSize(QSize(14, 14));
    m_moveUpBtn->setToolTip(TR("Monter le calque"));
    m_moveUpBtn->setFixedSize(26, 26);
    m_moveDownBtn = new QPushButton;
    m_moveDownBtn->setIcon(moveDownIcon());
    m_moveDownBtn->setIconSize(QSize(14, 14));
    m_moveDownBtn->setToolTip(TR("Descendre le calque"));
    m_moveDownBtn->setFixedSize(26, 26);
    m_applyAllBtn = new QPushButton(TR("Tous"));
    m_applyAllBtn->setCheckable(true);
    m_applyAllBtn->setToolTip(TR("Appliquer les modifications a tous les calques"));
    m_applyAllBtn->setFixedHeight(26);

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addWidget(m_duplicateBtn);
    btnLayout->addWidget(m_mergeBtn);
    btnLayout->addWidget(m_moveUpBtn);
    btnLayout->addWidget(m_moveDownBtn);
    btnLayout->addWidget(m_applyAllBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(m_addBtn, &QPushButton::clicked, this, &LayersPanel::onAddLayer);
    connect(m_removeBtn, &QPushButton::clicked, this, &LayersPanel::onRemoveLayer);
    connect(m_duplicateBtn, &QPushButton::clicked, this, &LayersPanel::onDuplicateLayer);
    connect(m_mergeBtn, &QPushButton::clicked, this, &LayersPanel::onMergeDown);
    connect(m_moveUpBtn, &QPushButton::clicked, this, &LayersPanel::onMoveUp);
    connect(m_moveDownBtn, &QPushButton::clicked, this, &LayersPanel::onMoveDown);
    connect(m_applyAllBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (m_document) m_document->setEditAllLayers(checked);
    });
    connect(m_layerList, &QListWidget::currentRowChanged, this, &LayersPanel::onLayerClicked);
    connect(m_layerList, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        if (!m_document) return;
        int row = m_layerList->row(item);
        int index = m_document->layerCount() - 1 - row;
        if (index >= 0 && index < m_document->layerCount()) {
            auto *layer = m_document->layerAt(index);
            bool visible = (item->checkState() == Qt::Checked);
            if (layer->isVisible() != visible)
                m_document->setLayerVisibility(index, visible);
        }
    });
    // Keep the document layer order in sync when the user drags rows around.
    connect(m_layerList->model(), &QAbstractItemModel::rowsMoved, this,
            [this]() { syncOrderFromList(); });
    connect(m_opacitySlider, &QSlider::valueChanged, this, &LayersPanel::onOpacityChanged);
    connect(m_blendModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LayersPanel::onBlendModeChanged);
}

void LayersPanel::setDocument(Document *doc) {
    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = doc;
    if (doc) {
        connect(doc, &Document::layersChanged, this, &LayersPanel::updateLayerList);
        // Keep thumbnails live on every content change (brush, bucket, effect...),
        // debounced so a continuous stroke doesn't rebuild them on every pixel.
        connect(doc, &Document::documentChanged, this, [this]() {
            m_thumbRefreshTimer->start();
        });
        connect(doc, &Document::activeLayerChanged, this, [this](int index) {
            m_layerList->blockSignals(true);
            // Layers are displayed in reverse order (top layer first)
            int row = m_document->layerCount() - 1 - index;
            m_layerList->setCurrentRow(row);
            m_layerList->blockSignals(false);

            auto *layer = m_document->layerAt(index);
            if (layer) {
                m_opacitySlider->blockSignals(true);
                m_opacitySlider->setValue(static_cast<int>(layer->opacity() * 100));
                m_opacitySlider->blockSignals(false);

                m_blendModeCombo->blockSignals(true);
                m_blendModeCombo->setCurrentIndex(static_cast<int>(layer->blendMode()));
                m_blendModeCombo->blockSignals(false);
            }
        });
        connect(doc, &Document::editScopeChanged, this, [this](bool editAllLayers) {
            m_applyAllBtn->blockSignals(true);
            m_applyAllBtn->setChecked(editAllLayers);
            m_applyAllBtn->blockSignals(false);
        });
        m_applyAllBtn->blockSignals(true);
        m_applyAllBtn->setChecked(doc->editAllLayers());
        m_applyAllBtn->blockSignals(false);
        updateLayerList();
    }
}

QIcon LayersPanel::makeThumbnail(const QImage &image) const {
    QImage thumb = image.scaled(40, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap pm(40, 30);
    pm.fill(Qt::white);
    QPainter tp(&pm);
    // Checkerboard behind, so transparency reads.
    for (int y2 = 0; y2 < 30; y2 += 5)
        for (int x2 = 0; x2 < 40; x2 += 5)
            tp.fillRect(x2, y2, 5, 5, ((x2/5 + y2/5) % 2 == 0) ? QColor(204,204,204) : Qt::white);
    tp.drawImage((40 - thumb.width()) / 2, (30 - thumb.height()) / 2, thumb);
    tp.end();
    return QIcon(pm);
}

void LayersPanel::updateLayerList() {
    if (!m_document) return;
    m_layerList->blockSignals(true);
    m_layerList->clear();

    // Show layers top-to-bottom (reverse order)
    for (int i = m_document->layerCount() - 1; i >= 0; --i) {
        auto *layer = m_document->layerAt(i);
        auto *item = new QListWidgetItem(layer->name());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setData(Qt::UserRole, i);  // remember which layer this row shows
        item->setCheckState(layer->isVisible() ? Qt::Checked : Qt::Unchecked);
        item->setIcon(makeThumbnail(layer->image()));
        if (!layer->isVisible())
            item->setForeground(Qt::gray);
        m_layerList->addItem(item);
    }

    int activeRow = m_document->layerCount() - 1 - m_document->activeLayerIndex();
    m_layerList->setCurrentRow(activeRow);
    m_layerList->blockSignals(false);
}

// Lightweight: refresh just the thumbnails of the existing rows (no rebuild, so
// no flicker or selection reset). Called whenever the document content changes
// — e.g. every brush/bucket edit — so thumbnails stay live.
void LayersPanel::refreshThumbnails() {
    if (!m_document) return;
    const int n = m_layerList->count();
    if (n != m_document->layerCount()) { updateLayerList(); return; }
    for (int row = 0; row < n; ++row) {
        auto *item = m_layerList->item(row);
        const int idx = item->data(Qt::UserRole).toInt();
        if (auto *layer = m_document->layerAt(idx))
            item->setIcon(makeThumbnail(layer->image()));
    }
}

void LayersPanel::showLayerContextMenu(const QPoint &pos) {
    if (!m_document) return;
    QListWidgetItem *item = m_layerList->itemAt(pos);
    if (!item) return;
    const int index = item->data(Qt::UserRole).toInt();
    Layer *layer = m_document->layerAt(index);
    if (!layer) return;
    m_document->setActiveLayer(index);

    QMenu menu(this);
    QAction *rename = menu.addAction(TR("Renommer..."));
    QAction *opacity = menu.addAction(TR("Opacité..."));
    menu.addSeparator();
    QAction *dup = menu.addAction(TR("Dupliquer le calque"));
    QAction *merge = menu.addAction(TR("Fusionner vers le bas"));
    QAction *del = menu.addAction(TR("Supprimer le calque"));

    QAction *chosen = menu.exec(m_layerList->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == rename) {
        bool ok;
        QString name = QInputDialog::getText(this, TR("Renommer le calque"),
                                             TR("Nom du calque :"), QLineEdit::Normal,
                                             layer->name(), &ok);
        if (ok && !name.isEmpty())
            m_document->setLayerName(index, name);
    } else if (chosen == opacity) {
        bool ok;
        int cur = static_cast<int>(layer->opacity() * 100);
        int v = QInputDialog::getInt(this, TR("Opacité du calque"),
                                     TR("Opacité (0-100 %) :"), cur, 0, 100, 1, &ok);
        if (ok)
            m_document->setLayerOpacity(index, v / 100.0f);
    } else if (chosen == dup) {
        m_document->duplicateLayer(index);
    } else if (chosen == merge) {
        mergeWithChoice(index);
    } else if (chosen == del) {
        m_document->removeLayer(index);
    }
}

void LayersPanel::onAddLayer() {
    if (m_document) m_document->addLayer();
}

void LayersPanel::onRemoveLayer() {
    if (m_document) m_document->removeLayer(m_document->activeLayerIndex());
}

void LayersPanel::onDuplicateLayer() {
    if (m_document) m_document->duplicateLayer(m_document->activeLayerIndex());
}

void LayersPanel::onMergeDown() {
    if (m_document) mergeWithChoice(m_document->activeLayerIndex());
}

// True if every pixel of the image is the same colour (a "flat"/background layer).
static bool isUniformColor(const QImage &imgIn) {
    QImage img = imgIn.convertToFormat(QImage::Format_ARGB32);
    if (img.width() == 0 || img.height() == 0) return true;
    const QRgb first = reinterpret_cast<const QRgb*>(img.constScanLine(0))[0];
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x)
            if (line[x] != first) return false;
    }
    return true;
}

// Smart merge: when merging a layer down, ask what to keep. If one of the two is
// a solid colour (a background), that's hinted so the user can keep the artwork.
void LayersPanel::mergeWithChoice(int topIndex) {
    if (!m_document) return;
    if (topIndex <= 0 || topIndex >= m_document->layerCount()) return;
    Layer *top = m_document->layerAt(topIndex);
    Layer *bottom = m_document->layerAt(topIndex - 1);
    if (!top || !bottom) return;

    const bool topFlat = isUniformColor(top->image());
    const bool bottomFlat = isUniformColor(bottom->image());

    // If exactly one layer is flat, the artwork choice is obvious enough to skip
    // the dialog only when BOTH are non-trivial; otherwise always ask.
    QString hint;
    if (bottomFlat && !topFlat)      hint = TR("Le calque du dessous est une couleur unie (arrière-plan).");
    else if (topFlat && !bottomFlat) hint = TR("Le calque du dessus est une couleur unie (arrière-plan).");
    else if (!topFlat && !bottomFlat) hint = TR("Les deux calques contiennent un dessin.");

    QMessageBox box(this);
    box.setWindowTitle(TR("Fusionner les calques"));
    QString text = QString("<b>%1</b>").arg(TR("Comment fusionner les deux calques ?"));
    if (!hint.isEmpty()) text += "<br>" + hint;
    box.setText(text);
    QPushButton *combine = box.addButton(TR("Superposer (normal)"), QMessageBox::AcceptRole);
    QPushButton *knockout = box.addButton(TR("Garder les deux dessins"), QMessageBox::ActionRole);
    QPushButton *keepTop = box.addButton(TR("Garder le calque du dessus"), QMessageBox::ActionRole);
    QPushButton *keepBottom = box.addButton(TR("Garder le calque du dessous"), QMessageBox::ActionRole);
    box.addButton(TR("Annuler|bouton"), QMessageBox::RejectRole);
    // Default to the knock-out merge when the top layer's flat background would
    // otherwise hide the bottom artwork; otherwise a plain normal merge is safest
    // (a normal merge no longer silently removes a photo's uniform border).
    box.setDefaultButton((topFlat || (!topFlat && !bottomFlat)) ? knockout : combine);
    box.exec();
    QAbstractButton *c = box.clickedButton();

    if (c == combine) {
        // Plain SourceOver merge — nothing is knocked out.
        m_document->mergeLayerDown(topIndex, false);
    } else if (c == knockout) {
        // Knock out the top layer's flat/uniform background (e.g. white) so the
        // bottom layer's artwork shows through; a no-op for non-flat layers.
        m_document->mergeLayerDown(topIndex, true);
    } else if (c == keepTop) {
        // Discard the bottom layer, keep the top one in its place.
        m_document->removeLayer(topIndex - 1);
    } else if (c == keepBottom) {
        // Discard the top layer, keep the bottom one.
        m_document->removeLayer(topIndex);
    }
    // else: cancelled -> nothing.
}

void LayersPanel::onMoveUp() {
    if (!m_document) return;
    int idx = m_document->activeLayerIndex();
    if (idx < m_document->layerCount() - 1)
        m_document->moveLayer(idx, idx + 1);
}

void LayersPanel::onMoveDown() {
    if (!m_document) return;
    int idx = m_document->activeLayerIndex();
    if (idx > 0)
        m_document->moveLayer(idx, idx - 1);
}

void LayersPanel::onLayerClicked(int row) {
    if (!m_document || row < 0) return;
    int index = m_document->layerCount() - 1 - row;
    m_document->setActiveLayer(index);
    emit layerSelected(index);
}

void LayersPanel::onOpacityChanged(int value) {
    m_opacityLabel->setText(QString("%1%").arg(value));
    if (!m_document) return;
    m_document->setLayerOpacity(m_document->activeLayerIndex(), value / 100.0f);
}

void LayersPanel::onBlendModeChanged(int index) {
    if (!m_document) return;
    auto modes = Layer::allBlendModes();
    if (index >= 0 && index < modes.size())
        m_document->setLayerBlendMode(m_document->activeLayerIndex(), modes[index]);
}

void LayersPanel::onVisibilityToggled() {
    if (!m_document) return;
    auto *layer = m_document->activeLayer();
    if (layer) {
        m_document->setLayerVisibility(m_document->activeLayerIndex(), !layer->isVisible());
        updateLayerList();
    }
}

void LayersPanel::syncOrderFromList() {
    if (!m_document) return;
    const int n = m_layerList->count();
    if (n != m_document->layerCount()) { updateLayerList(); return; }

    // Rows are displayed top-to-bottom; document indices run bottom-to-top.
    // Read each row's original layer index and build the new bottom-to-top order.
    std::vector<int> newOrder;
    newOrder.reserve(n);
    for (int row = n - 1; row >= 0; --row)
        newOrder.push_back(m_layerList->item(row)->data(Qt::UserRole).toInt());

    int active = m_document->activeLayerIndex();
    m_document->reorderLayers(newOrder, active);
}

void LayersPanel::retranslate() {
    m_addBtn->setToolTip(TR("Ajouter un calque"));
    m_removeBtn->setToolTip(TR("Supprimer le calque"));
    m_duplicateBtn->setToolTip(TR("Dupliquer le calque"));
    m_mergeBtn->setToolTip(TR("Fusionner vers le bas"));
    m_moveUpBtn->setToolTip(TR("Monter le calque"));
    m_moveDownBtn->setToolTip(TR("Descendre le calque"));
    m_applyAllBtn->setText(TR("Tous"));
    m_applyAllBtn->setToolTip(TR("Appliquer les modifications a tous les calques"));
}
