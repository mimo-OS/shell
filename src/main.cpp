#include "shell.h"

#include <QApplication>
#include <QIcon>
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    // Prevent fractional rounding artifacts on high-DPI and scaled displays
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // QApplication rather than QGuiApplication: the status bar's application
    // menu and several context menus are QWidget menus.
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("cutefish-shell"));
    app.setDesktopFileName(QStringLiteral("cutefish-shell"));
    app.setOrganizationName(QStringLiteral("cutefishos"));

    // Ensure all Qt Quick windows request an RGBA surface format with an alpha buffer for compositor blur
    QQuickWindow::setDefaultAlphaBuffer(true);
    QQuickWindow::setTextRenderType(QQuickWindow::QtTextRendering);

    if (QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor") || QIcon::themeName() == QLatin1String("Crule-dark")) {
        QIcon::setThemeName(QStringLiteral("Cutefish"));
    }

    // The shell owns the session's panels; closing the last one must not quit.
    app.setQuitOnLastWindowClosed(false);

    Shell shell;

    if (!shell.registerServices())
        return -1;

    shell.start();

    return app.exec();
}
