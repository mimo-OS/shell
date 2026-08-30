#include "menuimporter.h"
#include "../libdbusmenuqt/dbusmenutypes_p.h"
#include "menuimporteradaptor.h"

#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusServiceWatcher>

#include <KWindowInfo>
#include <KX11Extras>
#include <KWindowSystem>

static const char *DBUS_SERVICE = "com.canonical.AppMenu.Registrar";
static const char *DBUS_OBJECT_PATH = "/com/canonical/AppMenu/Registrar";

static MenuImporter *s_menuImporter = nullptr;

MenuImporter *MenuImporter::instance()
{
    return s_menuImporter;
}

MenuImporter::MenuImporter(QObject *parent)
    : QObject(parent)
    , m_serviceWatcher(new QDBusServiceWatcher(this))
{
    s_menuImporter = this;

    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &MenuImporter::slotServiceUnregistered);
}

MenuImporter::~MenuImporter()
{
    if (s_menuImporter == this)
        s_menuImporter = nullptr;

    QDBusConnection::sessionBus().unregisterService(DBUS_SERVICE);
}

WId MenuImporter::windowForPid(uint pid) const
{
    if (pid == 0)
        return 0;

    // Newest registration wins: an application that opened a second window
    // registers it again with the same pid.
    WId result = 0;
    for (auto it = m_windowPids.cbegin(); it != m_windowPids.cend(); ++it) {
        if (it.value() == pid && it.key() >= result)
            result = it.key();
    }

    return result;
}

bool MenuImporter::connectToBus()
{
    if (!QDBusConnection::sessionBus().registerService(DBUS_SERVICE)) {
        return false;
    }
    new MenuImporterAdaptor(this);
    QDBusConnection::sessionBus().registerObject(DBUS_OBJECT_PATH, this);

    return true;
}

void MenuImporter::RegisterWindow(WId id, const QDBusObjectPath &path)
{
    QString classClass;

    if (KWindowSystem::isPlatformX11()) {
        KWindowInfo info(id, NET::WMWindowType, NET::WM2WindowClass);
        NET::WindowTypes mask = NET::AllTypesMask;
        auto type = info.windowType(mask);

        // Menu can try to register, right click in gimp for example
        if (type != NET::Unknown && (type & (NET::Menu | NET::DropdownMenu | NET::PopupMenu))) {
            return;
        }

        classClass = info.windowClassClass();
    }

    if (path.path().isEmpty()) // prevent bad dbusmenu usage
        return;

    QString service = message().service();

    m_windowClasses.insert(id, classClass);
    m_menuServices.insert(id, service);
    m_menuPaths.insert(id, path);

    if (QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface()) {
        const QDBusReply<uint> pid = iface->servicePid(service);
        if (pid.isValid())
            m_windowPids.insert(id, pid.value());
    }

    if (!m_serviceWatcher->watchedServices().contains(service)) {
        m_serviceWatcher->addWatchedService(service);
    }

    emit WindowRegistered(id, service, path);
}

void MenuImporter::UnregisterWindow(WId id)
{
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowClasses.remove(id);
    m_windowPids.remove(id);

    emit WindowUnregistered(id);
}

QString MenuImporter::GetMenuForWindow(WId id, QDBusObjectPath &path)
{
    path = m_menuPaths.value(id);
    return m_menuServices.value(id);
}

void MenuImporter::slotServiceUnregistered(const QString &service)
{
    WId id = m_menuServices.key(service);
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowClasses.remove(id);
    m_windowPids.remove(id);
    emit WindowUnregistered(id);
    m_serviceWatcher->removeWatchedService(service);
}
