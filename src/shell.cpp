#include "shell.h"
#include "desktopmanager.h"

#include "statusbar.h"
#include "controlcenterdialog.h"
#include "systemtray/systemtraymodel.h"
#include "appmenu/appmenumodel.h"
#include "appmenu/appmenuapplet.h"
#include "poweractions.h"
#include "notifications.h"
#include "backgroundhelper.h"
#include "appearance.h"
#include "brightness.h"
#include "battery.h"

#include "dockwindow.h"
#include "docksettings.h"

#include "notificationmanager.h"
#include "notificationsmodel.h"
#include "historymodel.h"
#include "notificationcenterdialog.h"
#include "notificationpopup.h"
#include "screenhelper.h"

#include "launcher.h"
#include "launchermodel.h"
#include "pagemodel.h"
#include "iconitem.h"
#include "appmanager.h"
#include "applicationregistry.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QDBusConnection>
#include <QGuiApplication>
#include <QTranslator>
#include <QPixmapCache>
#include <QLocale>
#include <QFile>
#include <QDebug>

struct ServiceRegistration {
    const char *service;
    const char *path;
};

// Keep the service names stable for clients on the session bus.
static const ServiceRegistration s_services[] = {
    { "com.cutefish.Shell",     "/Shell"     },
    { "com.cutefish.Statusbar", "/Statusbar" },
    { "com.cutefish.Dock",      "/Dock"      },
    { "com.cutefish.Launcher",  "/Launcher"  },
    { "com.cutefish.Notification", "/Notification" },
};

Shell::Shell(QObject *parent)
    : QObject(parent)
    , m_engine(new QQmlEngine(this))
    , m_statusBar(nullptr)
    , m_dock(nullptr)
    , m_launcher(nullptr)
    , m_desktops(nullptr)
    , m_notifications(nullptr)
{
    QPixmapCache::setCacheLimit(2048);

    registerQmlTypes();
    loadTranslations();
}

Shell::~Shell()
{
    delete m_notifications;
    delete m_desktops;
    delete m_launcher;
    delete m_dock;
    delete m_statusBar;
}

void Shell::registerQmlTypes()
{
    registerApplicationsQmlTypes();

    const char *statusBarUri = "Cutefish.StatusBar";
    qmlRegisterType<SystemTrayModel>(statusBarUri, 1, 0, "SystemTrayModel");
    qmlRegisterType<ControlCenterDialog>(statusBarUri, 1, 0, "ControlCenterDialog");
    qmlRegisterType<Appearance>(statusBarUri, 1, 0, "Appearance");
    qmlRegisterType<Brightness>(statusBarUri, 1, 0, "Brightness");
    qmlRegisterType<Battery>(statusBarUri, 1, 0, "Battery");
    qmlRegisterType<AppMenuModel>(statusBarUri, 1, 0, "AppMenuModel");
    qmlRegisterType<AppMenuApplet>(statusBarUri, 1, 0, "AppMenuApplet");
    qmlRegisterType<PowerActions>(statusBarUri, 1, 0, "PowerActions");
    qmlRegisterType<Notifications>(statusBarUri, 1, 0, "Notifications");
    qmlRegisterType<BackgroundHelper>(statusBarUri, 1, 0, "BackgroundHelper");

    const char *notificationUri = "Cutefish.Notification";
    qmlRegisterType<NotificationsModel>(notificationUri, 1, 0, "NotificationsModel");
    qmlRegisterType<HistoryModel>(notificationUri, 1, 0, "HistoryModel");
    qmlRegisterType<ScreenHelper>(notificationUri, 1, 0, "ScreenHelper");
    qmlRegisterType<NotificationPopup>(notificationUri, 1, 0, "NotificationPopup");
    qmlRegisterType<NotificationCenterDialog>(notificationUri, 1, 0, "NotificationCenterDialog");

    qmlRegisterType<DockSettings>("Cutefish.Dock", 1, 0, "DockSettings");

    const char *launcherUri = "Cutefish.Launcher";
    qmlRegisterType<LauncherModel>(launcherUri, 1, 0, "LauncherModel");
    qmlRegisterType<PageModel>(launcherUri, 1, 0, "PageModel");
    qmlRegisterType<IconItem>(launcherUri, 1, 0, "IconItem");
    qmlRegisterType<AppManager>(launcherUri, 1, 0, "AppManager");
    qmlRegisterAnonymousType<QAbstractItemModel>(launcherUri, 0);

    // The desktop's types come from the org.cutefish.filemanager.desktop QML
    // plugin; the shell never registers them itself.
}

void Shell::loadTranslations()
{
    const QString locale = QLocale::system().name();

    // One catalogue per merged component, so the components' existing
    // translations are reused as they are.
    const QStringList components = { QStringLiteral("statusbar"),
                                     QStringLiteral("dock"),
                                     QStringLiteral("launcher"),
                                     QStringLiteral("notification") };

    for (const QString &component : components) {
        const QString path = QStringLiteral("/usr/share/cutefish-shell/translations/%1_%2.qm")
                                 .arg(component, locale);

        if (!QFile::exists(path))
            continue;

        QTranslator *translator = new QTranslator(this);
        if (translator->load(path))
            QGuiApplication::installTranslator(translator);
        else
            translator->deleteLater();
    }
}

bool Shell::registerServices()
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    for (const ServiceRegistration &registration : s_services) {
        if (!bus.registerService(QString::fromLatin1(registration.service))) {
            qWarning() << "cutefish-shell: cannot take the D-Bus name"
                       << registration.service << "- is a shell already running?";
            return false;
        }
    }

    return true;
}

void Shell::start()
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    // Construct the dock first so its D-Bus object and geometry are available
    // to the desktop, but on Wayland defer mapping it until after the desktop.
    // DesktopSurface is a full-screen ordinary xdg-toplevel; mapping it last
    // can leave it above the dock/status bar and consume every pointer click.
    m_dock = new DockWindow(m_engine);
    bus.registerObject(QStringLiteral("/Dock"), m_dock);

    m_desktops = new DesktopManager(m_engine, this);

    // The notification service comes up before the status bar: the status bar's
    // QML holds the notification centre window and reads the models the
    // manager publishes on the engine's root context.
    m_notifications = new NotificationManager(m_engine, this);
    bus.registerObject(QStringLiteral("/Notification"), m_notifications);

    m_statusBar = new StatusBar(m_engine);
    bus.registerObject(QStringLiteral("/Statusbar"), m_statusBar);

    m_launcher = new Launcher(m_engine);
    bus.registerObject(QStringLiteral("/Launcher"),
                       QStringLiteral("com.cutefish.Launcher"), m_launcher);
    m_launcher->setDock(m_dock);
    m_dock->setLauncher(m_launcher);

    m_dock->show();

}
