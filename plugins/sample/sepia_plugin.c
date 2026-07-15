/*
 * Sample paint.software plugin: a sepia-tone effect with an "Intensity" slider.
 *
 * Build (Linux):
 *   gcc -shared -fPIC -I../../src/plugins sepia_plugin.c -o sample_sepia_plugin.so
 * then drop the .so into your plugins folder (Effets ▸ Plugins ▸ Ouvrir le dossier…).
 *
 * The whole contract is plain C — no Qt, no C++.
 */
#include "plugin_api.h"

static int clamp(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

/* Straight-alpha RGBA8, edited in place. values[0] = intensity 0..100. */
static void sepia_process(unsigned char *rgba, int width, int height,
                          const int *values, int valueCount, void *userData) {
    (void)userData;
    int intensity = (valueCount > 0) ? values[0] : 80;
    if (intensity < 0) intensity = 0;
    if (intensity > 100) intensity = 100;

    long count = (long)width * (long)height;
    for (long i = 0; i < count; ++i) {
        unsigned char *p = rgba + i * 4;
        int r = p[0], g = p[1], b = p[2];
        int gray = (r * 299 + g * 587 + b * 114) / 1000;
        int sr = clamp(gray + 40);
        int sg = clamp(gray + 20);
        int sb = clamp(gray - 20);
        /* blend original toward sepia by intensity% */
        p[0] = (unsigned char)((r * (100 - intensity) + sr * intensity) / 100);
        p[1] = (unsigned char)((g * (100 - intensity) + sg * intensity) / 100);
        p[2] = (unsigned char)((b * (100 - intensity) + sb * intensity) / 100);
        /* p[3] (alpha) left untouched */
    }
}

static const PswParam sepia_params[] = {
    { "Intensité", 0, 100, 80, "%" }
};

static const PswEffect sepia_effect = {
    "Sépia (plugin externe)",   /* name shown in Effets ▸ Plugins */
    "Photo",                    /* category hint */
    1,                          /* paramCount */
    sepia_params,               /* params */
    sepia_process,              /* process */
    0                           /* userData */
};

PSW_EXPORT int psw_register_plugin(int abiVersion, void *host, PswAddEffectFn add) {
    if (abiVersion != PSW_PLUGIN_ABI_VERSION) return 1;   /* refuse mismatched host */
    add(host, &sepia_effect);
    return 0;
}
