#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

class PluginEffect;
class QLibrary;

// Discovers and loads external effect plugins (shared libraries exporting
// psw_register_plugin). Entirely additive: if no plugin directory exists or a
// library is not a valid plugin, it is silently skipped and the app is unaffected.
class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager();

    // Default locations searched for plugins: <appdir>/plugins and the writable
    // app-data plugins directory. Both are optional.
    static QStringList defaultPluginDirs();

    // Scan the given directories and load every valid plugin found. Safe to call
    // once. Never throws.
    void loadFrom(const QStringList &dirs);

    const std::vector<std::shared_ptr<PluginEffect>> &effects() const { return m_effects; }
    bool empty() const { return m_effects.empty(); }

private:
    void tryLoad(const QString &path);

    // Registration context passed through the C ABI as the opaque `host` pointer.
    struct RegisterCtx;
    static void addEffectTrampoline(void *host, const struct PswEffect *effect);

    std::vector<std::shared_ptr<QLibrary>> m_libraries;
    std::vector<std::shared_ptr<PluginEffect>> m_effects;
};
