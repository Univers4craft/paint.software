#pragma once

#include <QWidget>
#include <QVector>

class QHBoxLayout;
class Document;

// The open-image thumbnail strip shown at the right of the toolbar. Behaves
// like browser tabs: one thumbnail per open image, click to switch, X to close.
class ImageListBar : public QWidget {
    Q_OBJECT
public:
    explicit ImageListBar(QWidget *parent = nullptr);

    void setDocuments(const QVector<Document*> &docs, int activeIndex);

signals:
    void imageSelected(int index);
    void imageCloseRequested(int index);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void rebuild();

    QHBoxLayout *m_layout;
    QVector<Document*> m_docs;
    int m_active = 0;
};
