/*
 * paint.software plugin API — stable C ABI.
 *
 * A plugin is a shared library (.so / .dll / .dylib) that exports a single
 * function, psw_register_plugin(). The host calls it once at startup; the plugin
 * registers one or more effects by calling the host-provided add() callback.
 *
 * The boundary is deliberately plain C (POD structs + function pointers) so it is
 * ABI-stable across compilers and STL versions — never pass C++ objects across it.
 *
 * Image format: process() receives the image as straight-alpha RGBA8, i.e. 4 bytes
 * per pixel in byte order R, G, B, A, row-major, tightly packed (stride == width*4).
 * Edit the buffer in place.
 *
 * To write a plugin, include this header and implement psw_register_plugin().
 */
#ifndef PAINTSW_PLUGIN_API_H
#define PAINTSW_PLUGIN_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* Bump only on incompatible ABI changes. The host refuses mismatched plugins. */
#define PSW_PLUGIN_ABI_VERSION 1

/* Export marker for the entry point so it stays visible regardless of the
 * plugin's default symbol visibility. */
#if defined(_WIN32) || defined(__CYGWIN__)
#  define PSW_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#  define PSW_EXPORT __attribute__((visibility("default")))
#else
#  define PSW_EXPORT
#endif

/* One integer parameter, shown as a labelled slider with live preview. */
typedef struct PswParam {
    const char *label;   /* UTF-8 label shown in the dialog */
    int minValue;
    int maxValue;
    int defValue;
    const char *suffix;  /* optional, e.g. "%"; may be NULL */
} PswParam;

/*
 * Process an image in place.
 *   rgba        : straight-alpha RGBA8 pixels, length width*height*4, stride width*4
 *   width,height: image dimensions in pixels
 *   values      : current integer values, one per declared parameter
 *   valueCount  : length of values (== paramCount)
 *   userData    : the opaque pointer from PswEffect.userData
 */
typedef void (*PswProcessFn)(unsigned char *rgba, int width, int height,
                             const int *values, int valueCount, void *userData);

/* One effect provided by a plugin. */
typedef struct PswEffect {
    const char *name;       /* UTF-8 name shown in the Effects ▸ Plugins menu */
    const char *category;   /* optional submenu hint (informational); may be NULL */
    int paramCount;         /* number of parameters (0 = applies immediately) */
    const PswParam *params; /* array of paramCount PswParam; may be NULL if 0 */
    PswProcessFn process;   /* required */
    void *userData;         /* opaque, passed back to process(); may be NULL */
} PswEffect;

/*
 * Registrar callback supplied by the host. The plugin calls it once per effect.
 * The host copies everything it needs during the call, so the PswEffect (and the
 * strings it points to) need only remain valid for the duration of the call.
 */
typedef void (*PswAddEffectFn)(void *host, const PswEffect *effect);

/*
 * Entry point every plugin MUST export.
 *   abiVersion : the host's PSW_PLUGIN_ABI_VERSION — reject if it differs
 *   host       : opaque, pass back to add()
 *   add        : call once per effect to register it
 * Return 0 on success, non-zero to signal the plugin should be ignored.
 */
PSW_EXPORT int psw_register_plugin(int abiVersion, void *host, PswAddEffectFn add);

#ifdef __cplusplus
}
#endif

#endif /* PAINTSW_PLUGIN_API_H */
