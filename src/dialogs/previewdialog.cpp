#include "previewdialog.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QPainter>

PreviewDialog::PreviewDialog(const QString &title, const QImage &source,
                             QVector<Param> params, Renderer renderer, QWidget *parent)
    : QDialog(parent), m_renderer(std::move(renderer)), m_params(std::move(params)) {
    setWindowTitle(title);

    // Downscale the source so the renderer runs on a small image on every drag.
    const int maxDim = 340;
    m_previewSource = (source.width() > maxDim || source.height() > maxDim)
        ? source.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        : source;

    auto *root = new QVBoxLayout(this);

    m_preview = new QLabel;
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(maxDim, 220);
    m_preview->setStyleSheet("border: 1px solid palette(mid);");
    root->addWidget(m_preview);

    auto *grid = new QGridLayout;
    for (int i = 0; i < m_params.size(); ++i) {
        const Param &p = m_params[i];
        auto *lab = new QLabel(p.label);
        auto *slider = new QSlider(Qt::Horizontal);
        slider->setRange(p.min, p.max);
        slider->setValue(p.value);
        slider->setMinimumWidth(180);
        auto *spin = new QSpinBox;
        spin->setRange(p.min, p.max);
        spin->setValue(p.value);
        if (!p.suffix.isEmpty()) spin->setSuffix(p.suffix);

        connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
        connect(slider, &QSlider::valueChanged, this, [this]() { updatePreview(); });

        grid->addWidget(lab, i, 0);
        grid->addWidget(slider, i, 1);
        grid->addWidget(spin, i, 2);
        m_sliders.push_back(slider);
        m_spins.push_back(spin);
    }
    root->addLayout(grid);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_params.size(); ++i)
            m_sliders[i]->setValue(m_params[i].value);
    });

    updatePreview();
}

QVector<int> PreviewDialog::values() const {
    QVector<int> v;
    v.reserve(m_spins.size());
    for (auto *s : m_spins) v.push_back(s->value());
    return v;
}

void PreviewDialog::updatePreview() {
    if (!m_renderer) return;
    QImage processed = m_renderer(m_previewSource, values());
    if (processed.isNull()) return;

    // Composite over a checkerboard so transparency is visible in the preview.
    QImage canvas(processed.size(), QImage::Format_ARGB32);
    const int cell = 8;
    for (int y = 0; y < canvas.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(canvas.scanLine(y));
        for (int x = 0; x < canvas.width(); ++x) {
            bool light = ((x / cell) + (y / cell)) % 2 == 0;
            line[x] = light ? qRgb(200, 200, 200) : qRgb(160, 160, 160);
        }
    }
    QPainter p(&canvas);
    p.drawImage(0, 0, processed);
    p.end();
    m_preview->setPixmap(QPixmap::fromImage(canvas));
}
