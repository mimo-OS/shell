#include "shell.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    // QApplication rather than QGuiApplication: the status bar's application
    // menu and several context menus are QWidget menus.
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("cutefish-shell"));
    app.setDesktopFileName(QStringLiteral("cutefish-shell"));
    app.setOrganizationName(QStringLiteral("cutefishos"));

    // The shell owns the session's panels; closing the last one must not quit.
    app.setQuitOnLastWindowClosed(false);

    Shell shell;

    if (!shell.registerServices())
        return -1;

    shell.start();

    return app.exec();
}
