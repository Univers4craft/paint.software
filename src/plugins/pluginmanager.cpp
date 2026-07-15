#include "pluginmanager.h"
#include "plugineffect.h"
#include "plugins/plugin_api.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QSet>
#include <QStandardPaths>

// Registration context threaded through the C ABI as the opaque `host` pointer.
struct PluginManager::RegisterCtx {
    PluginManager *manager;
    std::shared_ptr<QLibrary> library;
    std::vector<std::shared_ptr<PluginEffect>> collected;
};

PluginManager::~PluginManager() {
    // Effects hold a shared_ptr to their QLibrary; drop effects before libraries so
    // no code is executed after its library is unloaded.
    m_effects.clear();
    m_libraries.clear();
}

QStringList PluginManager::defaultPluginDirs() {
    QStringList dirs;
    const QString appDir = QCoreApplication::applicationDirPath();
    // Next to the executable (dev builds output the bundled sample here).
    dirs << appDir + QStringLiteral("/plugins");
    // System plugins installed by the package, e.g. /usr/lib/paint.software/plugins
    // when the binary lives in /usr/bin.
    dirs << QDir::cleanPath(appDir + QStringLiteral("/../lib/paint.software/plugins"));
    // The user's personal, writable plugins folder.
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appData.isEmpty())
        dirs << appData + QStringLiteral("/plugins");
    dirs.removeDuplicates();
    return dirs;
}

// C ABI callback: cannot capture, so context arrives via `host`.
void PluginManager::addEffectTrampoline(void *host, const PswEffect *effect) {
    auto *ctx = static_cast<RegisterCtx *>(host);
    if (!ctx || !effect || !effect->process) return;   // reject malformed effects
    ctx->collected.push_back(std::make_shared<PluginEffect>(*effect, ctx->library));
}

void PluginManager::tryLoad(const QString &path) {
    auto library = std::make_shared<QLibrary>(path);
    if (!library->load())
        return;   // not a loadable shared library

    auto reg = reinterpret_cast<int (*)(int, void *, PswAddEffectFn)>(
        library->resolve("psw_register_plugin"));
    if (!reg) {
        library->unload();
        return;   // a shared library, but not one of our plugins
    }

    RegisterCtx ctx{this, library, {}};
    int rc = 1;
    // The plugin's registrar runs arbitrary native code; guard against C++
    // exceptions escaping (a hard crash inside a plugin is the plugin's own fault).
    try {
        rc = reg(PSW_PLUGIN_ABI_VERSION, &ctx, &PluginManager::addEffectTrampoline);
    } catch (...) {
        rc = 1;
    }

    if (rc != 0 || ctx.collected.empty()) {
        library->unload();
        return;   // ABI mismatch or nothing registered
    }

    m_libraries.push_back(library);
    for (auto &e : ctx.collected)
        m_effects.push_back(e);
}

void PluginManager::loadFrom(const QStringList &dirs) {
    static const QStringList kNameFilters{
        QStringLiteral("*.so"), QStringLiteral("*.dll"), QStringLiteral("*.dylib")};

    // Skip a plugin whose file name we've already loaded, so the same plugin
    // present in several search directories doesn't appear twice in the menu.
    QSet<QString> seen;
    for (const QString &dirPath : dirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        const QFileInfoList files = dir.entryInfoList(kNameFilters, QDir::Files, QDir::Name);
        for (const QFileInfo &fi : files) {
            if (seen.contains(fi.fileName())) continue;
            seen.insert(fi.fileName());
            tryLoad(fi.absoluteFilePath());
        }
    }
}
