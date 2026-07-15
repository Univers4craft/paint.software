#pragma once

#include <QDialog>
#include <QVector>
#include <QImage>
#include <functional>

class QLabel;
class QSlider;
class QSpinBox;

// A reusable parametric dialog: a set of labelled sliders driving a live preview
// of an image operation. Replaces the old chained QInputDialog popups (no preview).
//
// Usage: give it the source image, the parameter list, and a renderer that turns
// (source, values) into a processed image. After exec()==Accepted, read values().
class PreviewDialog : public QDialog {
    Q_OBJECT
public:
    struct Param {
        QString label;
        int min;
        int max;
        int value;
        QString suffix;   // optional, e.g. "%"
    };
    using Renderer = std::function<QImage(const QImage &, const QVector<int> &)>;

    PreviewDialog(const QString &title, const QImage &source,
                  QVector<Param> params, Renderer renderer, QWidget *parent = nullptr);

    QVector<int> values() const;

private:
    void updatePreview();

    QImage m_previewSource;   // downscaled source, for a responsive preview
    Renderer m_renderer;
    QVector<Param> m_params;
    QVector<QSlider *> m_sliders;
    QVector<QSpinBox *> m_spins;
    QLabel *m_preview = nullptr;
};
