#pragma once

#include <QIcon>
#include "tools/tool.h"

namespace ToolIcons {
    QIcon forTool(ToolType type);
    QIcon newDoc();
    QIcon openDoc();
    QIcon saveDoc();
    QIcon undoAction();
    QIcon redoAction();
    QIcon cutAction();
    QIcon copyAction();
    QIcon pasteAction();
    QIcon printAction();
    QIcon cropAction();
    QIcon deselectAction();
    QIcon pixelGridAction();
    QIcon rulersAction();
    // Utility-window icons shown on the right of the menu bar
    QIcon toolsWindow();
    QIcon historyWindow();
    QIcon layersWindow();
    QIcon colorsWindow();
    QIcon settings();
    QIcon help();
}
