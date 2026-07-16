#include "imagelistbar.h"
#include "../i18n.h"
#include "core/document.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QPainter>
#include <QFileInfo>
#include <QMouseEvent>
#include <QEvent>

namespace {
constexpr int kThumb = 38;   // thumbnail button size
constexpr int kImage = 30;   // rendered image size inside it
}

ImageListBar::ImageListBar(QWidget *parent) : QWidget(parent) {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(2, 1, 2, 1);
    m_layout->setSpacing(2);
}

void ImageListBar::setDocuments(const QVector<Document*> &docs, int activeIndex) {
    m_docs = docs;
    m_active = activeIndex;
    rebuild();
}

void ImageListBar::rebuild() {
    // Clear existing buttons.
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *w = item->widget()) w->deleteLater();
        delete item;
    }

    for (int i = 0; i < m_docs.size(); ++i) {
        Document *doc = m_docs[i];
        if (!doc) continue;

        auto *btn = new QToolButton(this);
        btn->setFixedSize(kThumb, kThumb);
        btn->setCheckable(true);
        btn->setChecked(i == m_active);
        btn->setAutoRaise(true);

        // Render the flattened image onto a checkerboard, scaled to fit.
        QPixmap pm(kImage, kImage);
        pm.fill(Qt::transparent);
        {
            QPainter p(&pm);
            for (int y = 0; y < kImage; y += 6)
                for (int x = 0; x < kImage; x += 6)
                    p.fillRect(x, y, 6, 6,
                               ((x / 6 + y / 6) % 2 == 0) ? QColor(255, 255, 255) : QColor(205, 205, 205));
            QImage thumb = doc->flatten().scaled(kImage, kImage, Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
            p.drawImage((kImage - thumb.width()) / 2, (kImage - thumb.height()) / 2, thumb);
            p.setPen(QPen(QColor(120, 120, 120), 1));
            p.drawRect(0, 0, kImage - 1, kImage - 1);
        }
        btn->setIcon(QIcon(pm));
        btn->setIconSize(QSize(kImage, kImage));

        QString name = doc->filePath().isEmpty() ? TR("Sans titre")
                                                 : QFileInfo(doc->filePath()).fileName();
        if (doc->isModified()) name += " *";
        btn->setToolTip(QString(TR("%1\n%2 × %3\n(clic milieu pour fermer)"))
                            .arg(name).arg(doc->width()).arg(doc->height()));

        connect(btn, &QToolButton::clicked, this, [this, i]() { emit imageSelected(i); });
        btn->setProperty("imageIndex", i);
        btn->installEventFilter(this);   // middle-click closes the image
        m_layout->addWidget(btn);
    }
}

bool ImageListBar::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::MiddleButton) {
            bool ok = false;
            int index = obj->property("imageIndex").toInt(&ok);
            if (ok) {
                emit imageCloseRequested(index);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
