cmake_minimum_required(VERSION 3.16)
project(PaintDali VERSION 1.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui PrintSupport)

set(CORE_SOURCES
    src/core/document.cpp
    src/core/layer.cpp
    src/core/selection.cpp
    src/core/history.cpp
)

set(CANVAS_SOURCES
    src/canvas/canvaswidget.cpp
)

set(TOOLS_SOURCES
    src/tools/tool.cpp
    src/tools/brushtool.cpp
    src/tools/erasertool.cpp
    src/tools/penciltool.cpp
    src/tools/filltool.cpp
    src/tools/colorpickertool.cpp
    src/tools/recolortool.cpp
    src/tools/selectiontool.cpp
    src/tools/lassotool.cpp
    src/tools/movetool.cpp
    src/tools/selectionhandles.cpp
    src/tools/moveselectiontool.cpp
    src/tools/zoomtool.cpp
    src/tools/pantool.cpp
    src/tools/texttool.cpp
    src/tools/shapetool.cpp
    src/tools/linetool.cpp
    src/tools/gradienttool.cpp
    src/tools/clonestamptool.cpp
    src/tools/magicwandtool.cpp
)

set(PANELS_SOURCES
    src/panels/layerspanel.cpp
    src/panels/colorspanel.cpp
    src/panels/historypanel.cpp
    src/panels/tooloptionspanel.cpp
    src/panels/imagelistbar.cpp
)

set(EFFECTS_SOURCES
    src/effects/effect.cpp
    src/effects/blureffect.cpp
    src/effects/sharpeneffect.cpp
    src/effects/noiseeffect.cpp
    src/effects/embosseffect.cpp
    src/effects/edgedetecteffect.cpp
    src/effects/oilpainteffect.cpp
    src/effects/pixelateeffect.cpp
    src/effects/motionblureffect.cpp
    src/effects/radialblureffect.cpp
    src/effects/zoomblureffect.cpp
    src/effects/surfaceblureffect.cpp
    src/effects/unfocuseffect.cpp
    src/effects/fragmenteffect.cpp
    src/effects/bulgeeffect.cpp
    src/effects/twisteffect.cpp
    src/effects/frostedglasseffect.cpp
    src/effects/crystalizeeffect.cpp
    src/effects/tileeffect.cpp
    src/effects/dentseffect.cpp
    src/effects/polarinversioneffect.cpp
    src/effects/medianeffect.cpp
    src/effects/reducenoiseeffect.cpp
    src/effects/quantizeeffect.cpp
    src/effects/gloweffect.cpp
    src/effects/redeyeremoveeffect.cpp
    src/effects/softenportraiteffect.cpp
    src/effects/vignetteeffect.cpp
    src/effects/turbulenceeffect.cpp
    src/effects/reliefeffect.cpp
    src/effects/outlineeffect.cpp
    src/effects/morphologyeffect.cpp
    src/effects/dropshadoweffect.cpp
)

set(ADJUSTMENTS_SOURCES
    src/adjustments/adjustment.cpp
    src/adjustments/brightnesscontrast.cpp
    src/adjustments/huesaturation.cpp
    src/adjustments/levels.cpp
    src/adjustments/curves.cpp
    src/adjustments/invertcolors.cpp
    src/adjustments/sepia.cpp
    src/adjustments/posterize.cpp
    src/adjustments/threshold.cpp
    src/adjustments/desaturate.cpp
    src/adjustments/colorbalance.cpp
    src/adjustments/exposure.cpp
    src/adjustments/highlightsshadows.cpp
    src/adjustments/temperaturetint.cpp
    src/adjustments/invertalpha.cpp
)

set(DIALOGS_SOURCES
    src/dialogs/resizedialog.cpp
    src/dialogs/canvassizedialog.cpp
    src/dialogs/newdocumentdialog.cpp
    src/dialogs/huesaturationdialog.cpp
    src/dialogs/layerpropertiesdialog.cpp
    src/dialogs/curvesdialog.cpp
    src/dialogs/settingsdialog.cpp
    src/dialogs/previewdialog.cpp
)

set(PLUGINS_SOURCES
    src/plugins/pluginmanager.cpp
    src/plugins/plugineffect.cpp
)

set(MAIN_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/toolicons.cpp
    src/i18n.cpp
    src/theme.cpp
    src/dockdropoverlay.cpp
)

set(RESOURCES
    src/resources/resources.qrc
)

set(ALL_SOURCES
    ${MAIN_SOURCES}
    ${CORE_SOURCES}
    ${CANVAS_SOURCES}
    ${TOOLS_SOURCES}
    ${PANELS_SOURCES}
    ${EFFECTS_SOURCES}
    ${ADJUSTMENTS_SOURCES}
    ${DIALOGS_SOURCES}
    ${PLUGINS_SOURCES}
    ${RESOURCES}
)

add_executable(paintdotnet ${ALL_SOURCES})

target_include_directories(paintdotnet PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(paintdotnet PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Core
    Qt6::Gui
)

install(TARGETS paintdotnet DESTINATION bin)

# ---- Test harness (headless functional tests) ----
set(NONMAIN_SOURCES
    ${CORE_SOURCES}
    ${CANVAS_SOURCES}
    ${TOOLS_SOURCES}
    ${PANELS_SOURCES}
    ${EFFECTS_SOURCES}
    ${ADJUSTMENTS_SOURCES}
    ${DIALOGS_SOURCES}
    ${PLUGINS_SOURCES}
    src/toolicons.cpp
    src/i18n.cpp
    src/theme.cpp
    ${RESOURCES}
)

add_executable(paintdotnet_tests EXCLUDE_FROM_ALL
    tests/test_all.cpp
    ${NONMAIN_SOURCES}
)
target_include_directories(paintdotnet_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(paintdotnet_tests PRIVATE
    Qt6::Widgets Qt6::PrintSupport Qt6::Core Qt6::Gui)

# ---- Sample plugin (demonstrates the C ABI; built into build/plugins) ----
# A MODULE library is a shared object meant to be dlopen'd, never linked against.
add_library(sample_sepia_plugin MODULE plugins/sample/sepia_plugin.c)
target_include_directories(sample_sepia_plugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins)
set_target_properties(sample_sepia_plugin PROPERTIES
    AUTOMOC OFF                                                 # plain C, no Qt
    C_VISIBILITY_PRESET default                                 # keep psw_register_plugin exported
    PREFIX ""                                                   # -> sample_sepia_plugin.so
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# ---- Desktop integration (installed by the package) ----
install(FILES packaging/paint.software.desktop DESTINATION share/applications)
install(FILES src/resources/paintdali-logo.png
        DESTINATION share/pixmaps RENAME paint.software.png)
# Ship the sample plugin as a system plugin so a fresh install has a working example
# (loaded from /usr/lib/paint.software/plugins).
install(TARGETS sample_sepia_plugin LIBRARY DESTINATION lib/paint.software/plugins)

# ---- CPack: build a Debian package with `cpack -G DEB` ----
set(CPACK_PACKAGE_NAME "paint.software")
set(CPACK_PACKAGE_VENDOR "Univers4craft")
# CI passes -DPAINTSW_VERSION=<major.minor.commitcount> so every push to main
# produces a higher version that APT sees as an upgrade; otherwise use PROJECT_VERSION.
if(DEFINED PAINTSW_VERSION AND NOT PAINTSW_VERSION STREQUAL "")
  set(CPACK_PACKAGE_VERSION "${PAINTSW_VERSION}")
else()
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
endif()
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source Paint.NET clone for Linux (C++/Qt6)")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Univers4craft/paint.software")
set(CPACK_PACKAGE_CONTACT "Univers4craft <Univers4craft@users.noreply.github.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Univers4craft <Univers4craft@users.noreply.github.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
# Auto-detect the exact Qt/GL runtime dependencies from the built binary. This
# resolves the correct package names for the build distro (e.g. the libqt6*t64
# variants on Ubuntu 24.04), so we do not hard-code a fragile fallback list.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Qt renders the standard dialog buttons (OK / Cancel / Yes / No) from its own
# catalogue, which ships separately. Recommends (installed by default) rather than
# Depends: without it the app runs fine, those buttons just stay English.
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-translations-l10n")
# Say "not compatible with this system" in plain words rather than leaving the
# user with a raw dependency error. Only reachable via dpkg -i / double-click:
# apt refuses before the package is ever unpacked (see the script's comment).
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb-control/preinst")
set(CPACK_PACKAGE_FILE_NAME "paint.software_${CPACK_PACKAGE_VERSION}_amd64")
include(CPack)
