#include "plugineffect.h"
#include "../i18n.h"

#include <QImage>
#include <QLibrary>

PluginEffect::PluginEffect(const PswEffect &desc, std::shared_ptr<QLibrary> library)
    : m_process(desc.process), m_userData(desc.userData), m_library(std::move(library)) {
    // Deep-copy every string out of the plugin's memory: after registration we must
    // not rely on the plugin's pointers still being valid.
    m_name = desc.name ? QString::fromUtf8(desc.name) : QStringLiteral("Plugin");
    m_category = desc.category ? QString::fromUtf8(desc.category) : QString();

    const int n = desc.paramCount > 0 ? desc.paramCount : 0;
    m_params.reserve(n);
    m_values.reserve(n);
    for (int i = 0; i < n && desc.params; ++i) {
        const PswParam &p = desc.params[i];
        PluginParamInfo info;
        info.label = p.label ? QString::fromUtf8(p.label) : TR("Paramètre %1").arg(i + 1);
        info.minValue = p.minValue;
        info.maxValue = qMax(p.minValue, p.maxValue);
        info.defValue = qBound(info.minValue, p.defValue, info.maxValue);
        info.suffix = p.suffix ? QString::fromUtf8(p.suffix) : QString();
        m_params.push_back(info);
        m_values.push_back(info.defValue);
    }
}

QVector<int> PluginEffect::defaultValues() const {
    QVector<int> v;
    v.reserve(static_cast<int>(m_params.size()));
    for (const auto &p : m_params) v.push_back(p.defValue);
    return v;
}

QImage PluginEffect::apply(const QImage &input) {
    if (!m_process) return input;

    // Straight-alpha, byte-ordered RGBA (R,G,B,A) on every architecture, tightly
    // packed (stride == width*4 for a 4-byte format) — exactly what the ABI promises.
    QImage img = input.convertToFormat(QImage::Format_RGBA8888);
    if (img.isNull()) return input;

    QVector<int> values = m_values;
    if (values.size() != static_cast<int>(m_params.size()))
        values = defaultValues();

    m_process(img.bits(), img.width(), img.height(),
              values.constData(), values.size(), m_userData);
    return img;
}
