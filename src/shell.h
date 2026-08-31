#ifndef SHELL_H
#define SHELL_H

#include <QObject>

class QQmlEngine;
class StatusBar;
class MainWindow;
class Launcher;
class DesktopManager;
class NotificationManager;

/**
 * cutefish-shell: the status bar, the dock, the launcher and the desktop in one
 * process, drawing from one QQmlEngine.
 *
 * Each component keeps its own window (a ShellSurface) and its own D-Bus name,
 * so everything that used to talk to cutefish-statusbar, cutefish-dock or
 * cutefish-launcher still works unchanged; what they no longer pay for is four
 * QML engines, four copies of every QML type, and four icon caches.
 *
 * Startup order matters: the dock publishes the geometry the launcher and the
 * desktop lay themselves out against, so it comes first.
 */
class Shell : public QObject
{
    Q_OBJECT

public:
    explicit Shell(QObject *parent = nullptr);
    ~Shell() override;

    /** Registers the D-Bus names. Returns false if a shell is already running. */
    bool registerServices();

    void start();

private:
    void registerQmlTypes();
    void loadTranslations();

    QQmlEngine *m_engine;

    StatusBar *m_statusBar;
    MainWindow *m_dock;
    Launcher *m_launcher;
    DesktopManager *m_desktops;
    NotificationManager *m_notifications;
};

#endif // SHELL_H
