#include "menuimporter.h"
#include "../libdbusmenuqt/dbusmenutypes_p.h"
#include "menuimporteradaptor.h"

#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusServiceWatcher>

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

uint MenuImporter::windowForPid(uint pid) const
{
    if (pid == 0)
        return 0;

    // Newest registration wins: an application that opened a second window
    // registers it again with the same pid.
    uint result = 0;
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

void MenuImporter::RegisterWindow(uint id, const QDBusObjectPath &path)
{
    if (path.path().isEmpty()) // prevent bad dbusmenu usage
        return;

    QString service = message().service();

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

void MenuImporter::UnregisterWindow(uint id)
{
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowPids.remove(id);

    emit WindowUnregistered(id);
}

QString MenuImporter::GetMenuForWindow(uint id, QDBusObjectPath &path)
{
    path = m_menuPaths.value(id);
    return m_menuServices.value(id);
}

void MenuImporter::slotServiceUnregistered(const QString &service)
{
    const uint id = m_menuServices.key(service);
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowPids.remove(id);
    emit WindowUnregistered(id);
    m_serviceWatcher->removeWatchedService(service);
}
