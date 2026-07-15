#pragma once

#include "effects/effect.h"
#include "plugins/plugin_api.h"

#include <QString>
#include <QVector>
#include <memory>
#include <vector>

class QLibrary;

// Host-side parameter description (a deep copy of a PswParam so we never depend on
// the plugin's memory after registration).
struct PluginParamInfo {
    QString label;
    int minValue = 0;
    int maxValue = 100;
    int defValue = 0;
    QString suffix;
};

// Wraps a plugin-provided PswEffect as a first-class Effect so it flows through the
// exact same pipeline (PreviewDialog, applyImageOperationToTargetLayers) as built-in
// effects. Holds a reference to the QLibrary so the code stays mapped while in use.
class PluginEffect : public Effect {
public:
    PluginEffect(const PswEffect &desc, std::shared_ptr<QLibrary> library);

    QString name() const override { return m_name; }
    QString category() const { return m_category; }

    // Runs the plugin's process() on a straight-alpha RGBA8 copy using the current
    // parameter values, then returns the result (straight-alpha ARGB32).
    QImage apply(const QImage &input) override;

    const std::vector<PluginParamInfo> &params() const { return m_params; }
    int paramCount() const { return static_cast<int>(m_params.size()); }
    void setValues(const QVector<int> &values) { m_values = values; }
    QVector<int> defaultValues() const;

private:
    QString m_name;
    QString m_category;
    std::vector<PluginParamInfo> m_params;
    PswProcessFn m_process = nullptr;
    void *m_userData = nullptr;
    std::shared_ptr<QLibrary> m_library;   // keeps the plugin code loaded
    QVector<int> m_values;
};
