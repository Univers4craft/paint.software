#pragma once

#include <QWidget>
#include <QTimer>
#include <QIcon>
#include <QListWidget>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class Document;

class LayersPanel : public QWidget {
    Q_OBJECT
public:
    explicit LayersPanel(QWidget *parent = nullptr);

    void setDocument(Document *doc);
    // Re-applies button tooltips in the current language.
    void retranslate();

signals:
    void layerSelected(int index);

public slots:
    void updateLayerList();
    void refreshThumbnails();

private slots:
    void onAddLayer();
    void onRemoveLayer();
    void onDuplicateLayer();
    void onMergeDown();
    void onMoveUp();
    void onMoveDown();
    void onLayerClicked(int row);
    void onOpacityChanged(int value);
    void onBlendModeChanged(int index);
    void onVisibilityToggled();
    void syncOrderFromList();
    void showLayerContextMenu(const QPoint &pos);
    void mergeWithChoice(int topIndex);
    QIcon makeThumbnail(const QImage &image) const;

private:
    Document *m_document = nullptr;
    class QTimer *m_thumbRefreshTimer = nullptr;
    QListWidget *m_layerList;
    QSlider *m_opacitySlider;
    QLabel *m_opacityLabel;
    QComboBox *m_blendModeCombo;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_duplicateBtn;
    QPushButton *m_mergeBtn;
    QPushButton *m_moveUpBtn;
    QPushButton *m_moveDownBtn;
    QPushButton *m_applyAllBtn;
};
