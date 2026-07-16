#include "historypanel.h"
#include "../i18n.h"
#include "core/document.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QPainter>
#include <QPainterPath>

HistoryPanel::HistoryPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    m_historyList = new QListWidget;
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyList->setIconSize(QSize(16, 16));
    layout->addWidget(m_historyList);

    // Barre basse annuler/rétablir
    auto *bottomBar = new QHBoxLayout;
    bottomBar->setContentsMargins(0, 0, 0, 0);
    bottomBar->addStretch();
    // Standard undo/redo glyphs — instantly recognisable and crisp at any size.
    const QString btnStyle =
        "QToolButton { font-size: 18px; font-weight: bold; color: #4a84e0;"
        " border: 1px solid palette(mid); border-radius: 4px; }"
        "QToolButton:hover { background: rgba(74,132,224,0.18); border-color: #4a84e0; }"
        "QToolButton:pressed { background: rgba(74,132,224,0.35); }";
    auto *undoBtn = new QToolButton;
    undoBtn->setText(QString::fromUtf8("↶"));
    undoBtn->setFixedSize(30, 26);
    undoBtn->setStyleSheet(btnStyle);
    undoBtn->setToolTip(TR("Annuler (Ctrl+Z)"));
    auto *redoBtn = new QToolButton;
    redoBtn->setText(QString::fromUtf8("↷"));
    redoBtn->setFixedSize(30, 26);
    redoBtn->setStyleSheet(btnStyle);
    redoBtn->setToolTip(TR("Rétablir (Ctrl+Y)"));
    bottomBar->addWidget(undoBtn);
    bottomBar->addWidget(redoBtn);
    layout->addLayout(bottomBar);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("font-size: 9px; color: #888;");
    m_statusLabel->setVisible(false);

    connect(undoBtn, &QToolButton::clicked, this, [this]() {
        if (m_document) { m_document->history().undo(); }
    });
    connect(redoBtn, &QToolButton::clicked, this, [this]() {
        if (m_document) { m_document->history().redo(); }
    });

    // Click any entry to travel to that point in history (row 0 = Original).
    connect(m_historyList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!m_document || m_navigating) return;
        m_navigating = true;
        m_document->history().goToIndex(m_historyList->row(item));
        m_navigating = false;
    });
}

void HistoryPanel::setDocument(Document *doc) {
    m_document = doc;
    if (doc) {
        connect(&doc->history(), &HistoryManager::historyChanged, this, &HistoryPanel::updateHistoryList);
        updateHistoryList();
    }
}

void HistoryPanel::updateHistoryList() {
    if (!m_document) return;
    m_historyList->clear();

    auto *original = new QListWidgetItem("▶ Original");
    m_historyList->addItem(original);

    // Full timeline (done + still-redoable), so past AND future steps are shown.
    QStringList history = m_document->history().fullHistoryList();
    for (const QString &item : history) {
        m_historyList->addItem("▶ " + item);
    }

    int currentIdx = m_document->history().currentIndex();
    m_historyList->setCurrentRow(currentIdx);

    // Dim the "redo" items (those after the current position). Past/current
    // items are left with NO explicit colour so the active theme's text colour
    // applies — otherwise a hard-coded dark colour vanished on the dark theme.
    for (int i = 0; i < m_historyList->count(); ++i) {
        auto *item = m_historyList->item(i);
        if (i > currentIdx)
            item->setForeground(QColor(140, 140, 140));   // readable on light AND dark
        // else: inherit the stylesheet/theme text colour
    }
}
