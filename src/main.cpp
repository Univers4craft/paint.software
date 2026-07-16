#include <QApplication>
#include <QIcon>
#include <QFileInfo>
#include "mainwindow.h"
#include "i18n.h"

// Set by CMake; only a hand-rolled build without the definition lands here.
#ifndef PAINTSW_VERSION_STR
#define PAINTSW_VERSION_STR "dev"
#endif
#include "theme.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("paint.software");
    app.setApplicationVersion(QStringLiteral(PAINTSW_VERSION_STR));
    app.setOrganizationName("PaintDali");
    app.setWindowIcon(QIcon(":/paintdali-logo.png"));

    // Restore the saved language / colour scheme before any UI is built.
    I18n::loadFromSettings();
    I18n::applyQtTranslations();
    Theme::loadFromSettings();

    MainWindow window;
    window.show();

    // Open any images passed on the command line (each becomes an entry in the
    // image list, like paint.net).
    for (int i = 1; i < argc; ++i) {
        const QString path = QString::fromLocal8Bit(argv[i]);
        if (QFileInfo::exists(path))
            window.openFile(path);
    }

    return app.exec();
}
