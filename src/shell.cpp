#include "shell.h"

#include "core/qmltypes.h"
#include "core/services/processprovider.h"
#include "desktop/desktopmanager.h"
#include "dock/dockwindow.h"
#include "dock/qmltypes.h"
#include "launcher/launcher.h"
#include "launcher/qmltypes.h"
#include "notification/notificationmanager.h"
#include "notification/qmltypes.h"
#include "statusbar/qmltypes.h"
#include "statusbar/statusbar.h"

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

namespace {

struct ServiceRegistration {
    const char *service;
    const char *path;
};

// Keep the service names stable for clients on the session bus.
constexpr ServiceRegistration s_services[] = {
    { "com.cutefish.Shell",        "/Shell"        },
    { "com.cutefish.Statusbar",    "/Statusbar"    },
    { "com.cutefish.Dock",         "/Dock"         },
    { "com.cutefish.Launcher",     "/Launcher"     },
    { "com.cutefish.Notification", "/Notification" },
};

// One catalogue per merged component, so the components' existing translations
// are reused as they are.
constexpr const char *s_translationCatalogues[] = {
    "statusbar", "dock", "launcher", "notification"
};

} // namespace

#if defined(__linux__)
#include <malloc.h>
#endif
#include <QTimer>

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

#if defined(__linux__)
    QTimer *trimTimer = new QTimer(this);
    connect(trimTimer, &QTimer::timeout, this, []() {
        malloc_trim(0);
    });
    trimTimer->start(45000);
    QTimer::singleShot(3000, this, []() {
        malloc_trim(0);
    });
#endif

    registerQmlTypes();
    loadTranslations();

    // Shared by every component's QML; one instance on the root context rather
    // than one per window.
    m_engine->rootContext()->setContextProperty(QStringLiteral("process"),
                                                new ProcessProvider(this));
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
    registerCoreQmlTypes();
    registerStatusBarQmlTypes();
    registerDockQmlTypes();
    registerLauncherQmlTypes();
    registerNotificationQmlTypes();

    // The background is drawn from Cutefish.Appearance's Wallpaper type; the
    // desktop icons come from the org.cutefish.filemanager.desktop QML plugin,
    // and both register their own types.
}

void Shell::loadTranslations()
{
    const QString locale = QLocale::system().name();

    for (const char *component : s_translationCatalogues) {
        const QString path = QStringLiteral("/usr/share/cutefish-shell/translations/%1_%2.qm")
                                 .arg(QLatin1String(component), locale);

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
    m_statusBar->show();
}
