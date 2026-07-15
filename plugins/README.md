# paint.software plugins

paint.software can load **external effect plugins** — native shared libraries that
add new entries under **Effets ▸ Plugins**, exactly like Paint.NET's plugin model.

## Where plugins go
The app scans, at startup:
- `<application folder>/plugins/`
- the writable app-data folder: `~/.local/share/PaintDali/plugins/` (Linux)

Use **Effets ▸ Plugins ▸ Ouvrir le dossier des plugins…** to open (and create) that folder.

## Writing a plugin
A plugin is a shared library that includes [`plugin_api.h`](../src/plugins/plugin_api.h)
and exports one function, `psw_register_plugin()`. The whole contract is **plain C**
(no Qt, no C++), so it is ABI-stable across compilers.

Each effect provides:
- a **name** (and optional category),
- zero or more **integer parameters** (shown as sliders with **live preview**),
- a **`process()`** function that edits the image in place.

Images are passed as **straight-alpha RGBA8** (4 bytes/pixel, order R,G,B,A,
tightly packed, `stride == width*4`).

## Example
See [`sample/sepia_plugin.c`](sample/sepia_plugin.c) — a sepia effect with an
"Intensity" slider.

Build it (Linux):
```bash
cd plugins/sample
gcc -shared -fPIC -I../../src/plugins sepia_plugin.c -o sample_sepia_plugin.so
```
The project's CMake also builds it automatically into `build/plugins/`, so running
the app from the build directory picks it up with no extra steps.

## Notes
- **ABI version:** the current ABI is `1` (`PSW_PLUGIN_ABI_VERSION`). A plugin must
  reject a host whose version differs.
- **Safety:** native plugins run in-process and execute arbitrary code — only load
  plugins you trust, just like with Paint.NET.
