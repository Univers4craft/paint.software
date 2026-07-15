# paint.software plugins — developer guide

paint.software loads **external effect plugins**, exactly like Paint.NET: a plugin is
a small shared library that, once dropped into a plugins folder, adds a new entry
under **Effets ▸ Plugins** with its own parameters and **live preview**.

> 🇫🇷 **Résumé rapide** : un plugin est une bibliothèque `.so` écrite en **C** qui expose
> une fonction `psw_register_plugin`. Tu la compiles, tu la déposes dans le dossier des
> plugins, tu relances l'app → ton effet apparaît dans **Effets ▸ Plugins**. Détails plus bas.

---

## 1. How it works

```
        your_plugin.so                    paint.software
  ┌────────────────────────┐         ┌───────────────────────────┐
  │ psw_register_plugin()  │◄── dlopen + resolve ────────────────│  PluginManager
  │   → add(host, &effect) │──── PswEffect (name, params, ───────►│   → PluginEffect : Effect
  │ process(rgba, w, h, …) │        process fn) ─────────────────►│   → Effets ▸ Plugins menu
  └────────────────────────┘                                     │   → PreviewDialog (live)
                                                                 └───────────────────────────┘
```

1. At startup the app scans its plugin folders and `dlopen`s every shared library.
2. For each one it resolves the exported symbol **`psw_register_plugin`** and calls it.
3. Your plugin calls the host's `add()` callback once per effect it provides, passing a
   `PswEffect` (name, category, parameters, and a `process` function).
4. The host wraps each effect and adds it to **Effets ▸ Plugins**. When the user runs it,
   the parameters appear as sliders with a **live preview**, then `process()` is called on
   the real image.

The boundary is **plain C** (`src/plugins/plugin_api.h`) — no Qt, no C++ — so plugins are
ABI-stable across compilers.

---

## 2. Where plugins go

The app searches, at startup (a missing folder is simply skipped):

| Location | Purpose |
|---|---|
| `<app folder>/plugins/` | next to the executable — the built-in sample lives here in dev builds |
| `<app folder>/../lib/paint.software/plugins/` | system plugins from the `.deb` package (`/usr/lib/paint.software/plugins`) |
| `~/.local/share/PaintDali/paint.software/plugins/` | **your** personal plugins (writable) |

Use **Effets ▸ Plugins ▸ Ouvrir le dossier des plugins…** to open (and create) the personal
folder. The same plugin found in several folders is loaded only once.

> ⚠️ Plugins are loaded **once, at startup** — **restart the app** after adding a `.so`.

---

## 3. The ABI (`plugin_api.h`)

```c
#define PSW_PLUGIN_ABI_VERSION 1

typedef struct {                 /* one integer parameter -> one slider */
    const char *label;           /* shown in the dialog */
    int minValue, maxValue, defValue;
    const char *suffix;          /* e.g. "%"; may be NULL */
} PswParam;

/* Edit the image in place. rgba = straight-alpha RGBA8, order R,G,B,A,
   tightly packed (stride == width*4). values[] has one entry per parameter. */
typedef void (*PswProcessFn)(unsigned char *rgba, int width, int height,
                             const int *values, int valueCount, void *userData);

typedef struct {
    const char *name;            /* menu label */
    const char *category;        /* optional hint (informational) */
    int paramCount;
    const PswParam *params;      /* array of paramCount, or NULL */
    PswProcessFn process;        /* required */
    void *userData;              /* opaque, passed back to process() */
} PswEffect;

/* Every plugin exports this. Return 0 on success. */
PSW_EXPORT int psw_register_plugin(int abiVersion, void *host, PswAddEffectFn add);
```

**Image format:** you receive the pixels as **straight-alpha RGBA8** — 4 bytes per pixel in
the order **R, G, B, A**, row-major, tightly packed (`stride == width*4`), on every platform.
Alpha is *not* premultiplied. Edit the buffer in place; leave alpha (`p[3]`) alone unless your
effect changes transparency.

---

## 4. A complete example

`brightness.c` — a one-slider brightness effect:

```c
#include "plugin_api.h"

static int clamp8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

/* values[0] = brightness, -100..100 */
static void brightness_process(unsigned char *rgba, int w, int h,
                               const int *values, int n, void *user) {
    (void)user;
    int delta = (n > 0 ? values[0] : 0) * 255 / 100;   /* -255..255 */
    long count = (long)w * h;
    for (long i = 0; i < count; ++i) {
        unsigned char *p = rgba + i * 4;
        p[0] = (unsigned char)clamp8(p[0] + delta);    /* R */
        p[1] = (unsigned char)clamp8(p[1] + delta);    /* G */
        p[2] = (unsigned char)clamp8(p[2] + delta);    /* B */
        /* p[3] = alpha, untouched */
    }
}

static const PswParam params[] = {
    { "Luminosité", -100, 100, 0, "" }
};
static const PswEffect effect = {
    "Luminosité (plugin)", "Photo", 1, params, brightness_process, 0
};

PSW_EXPORT int psw_register_plugin(int abi, void *host, PswAddEffectFn add) {
    if (abi != PSW_PLUGIN_ABI_VERSION) return 1;   /* refuse a mismatched host */
    add(host, &effect);                            /* register it */
    return 0;
}
```

A single plugin may register **several** effects — just call `add()` more than once.

See also the ready-to-build [`sample/sepia_plugin.c`](sample/sepia_plugin.c).

---

## 5. Build it

You only need a C compiler and the header (`src/plugins/plugin_api.h`):

```bash
# Linux
gcc -shared -fPIC -I<repo>/src/plugins brightness.c -o brightness.so

# macOS
clang -shared -fPIC -I<repo>/src/plugins brightness.c -o brightness.dylib

# Windows (MinGW)
gcc -shared -I<repo>\src\plugins brightness.c -o brightness.dll
```

The project's CMake also builds the bundled sample automatically into `build/plugins/`,
so a dev build picks it up with zero steps.

---

## 6. Install & run

1. Copy your `brightness.so` into the plugins folder
   (**Effets ▸ Plugins ▸ Ouvrir le dossier des plugins…**, i.e.
   `~/.local/share/PaintDali/paint.software/plugins/`).
2. **Restart** paint.software.
3. Open **Effets ▸ Plugins** → your effect is there. Because it declares a parameter, a
   dialog opens with a **live-preview** slider; drag it, click OK, and it applies to the
   active layer (or the current selection).

---

## 7. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Effect doesn't appear | Wrong folder, or you didn't restart. Check the folders in §2. |
| Loads but nothing runs | Symbol not exported. Ensure the entry point is `PSW_EXPORT int psw_register_plugin(...)` and you compiled with default visibility. Verify with `nm -D your.so \| grep psw_register_plugin`. |
| Appears twice | The same file is in two folders — harmless, the app de-duplicates by filename. |
| Nothing happens for some images | `process()` must handle any `width`/`height`; remember the buffer is `width*height*4` bytes. |
| Colors look swapped | The buffer is **R,G,B,A** (not B,G,R,A). |

---

## 8. ABI stability & safety

- **ABI version** is `1` (`PSW_PLUGIN_ABI_VERSION`). Always reject a host whose version
  differs — the app skips plugins that return non-zero. The ABI will only change on a major
  release, and the version number bumps with it.
- **Security:** native plugins run **in-process** and execute arbitrary code, exactly like
  Paint.NET's. Only install plugins you trust.

---

## 🇫🇷 En résumé
Un plugin = une lib `.so` en **C** qui inclut `plugin_api.h` et exporte `psw_register_plugin`.
Il déclare un effet (`PswEffect` : nom, paramètres, fonction `process`) qui reçoit l'image en
**RGBA8 (R,G,B,A), alpha non prémultiplié**, à modifier sur place. Compile avec `gcc -shared -fPIC`,
dépose le `.so` dans le dossier des plugins, **relance** l'app → l'effet apparaît dans
**Effets ▸ Plugins** avec ses curseurs et l'aperçu en direct.
