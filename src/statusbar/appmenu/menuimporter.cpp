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

const MenuImporter::Registration *MenuImporter::newestFor(uint pid) const
{
    if (pid == 0)
        return nullptr;

    // An application that opened a second window registers it again with the
    // same pid; the last registration is the one the user is looking at.
    for (int i = m_registrations.count() - 1; i >= 0; --i) {
        if (m_registrations.at(i).pid == pid)
            return &m_registrations.at(i);
    }

    return nullptr;
}

QString MenuImporter::serviceForPid(uint pid) const
{
    const Registration *registration = newestFor(pid);
    return registration ? registration->service : QString();
}

QString MenuImporter::pathForPid(uint pid) const
{
    const Registration *registration = newestFor(pid);
    return registration ? registration->path.path() : QString();
}

void MenuImporter::removeAt(int index)
{
    const QString service = m_registrations.at(index).service;
    m_registrations.removeAt(index);

    bool stillExported = false;
    for (const Registration &registration : std::as_const(m_registrations)) {
        if (registration.service == service) {
            stillExported = true;
            break;
        }
    }

    if (!stillExported)
        m_serviceWatcher->removeWatchedService(service);
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

    const QString service = message().service();

    Registration registration;
    registration.windowId = id;
    registration.service = service;
    registration.path = path;

    if (QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface()) {
        const QDBusReply<uint> pid = iface->servicePid(service);
        if (pid.isValid())
            registration.pid = pid.value();
    }

    for (int i = 0; i < m_registrations.count(); ++i) {
        if (m_registrations.at(i).windowId == id && m_registrations.at(i).service == service) {
            m_registrations.removeAt(i);
            break;
        }
    }

    m_registrations.append(registration);

    if (!m_serviceWatcher->watchedServices().contains(service)) {
        m_serviceWatcher->addWatchedService(service);
    }

    emit WindowRegistered(id, service, path);
}

void MenuImporter::UnregisterWindow(uint id)
{
    const QString service = calledFromDBus() ? message().service() : QString();

    for (int i = m_registrations.count() - 1; i >= 0; --i) {
        const Registration &registration = m_registrations.at(i);
        if (registration.windowId != id)
            continue;
        if (!service.isEmpty() && registration.service != service)
            continue;

        removeAt(i);
        emit WindowUnregistered(id);
        return;
    }
}

QString MenuImporter::GetMenuForWindow(uint id, QDBusObjectPath &path)
{
    for (int i = m_registrations.count() - 1; i >= 0; --i) {
        const Registration &registration = m_registrations.at(i);
        if (registration.windowId == id) {
            path = registration.path;
            return registration.service;
        }
    }

    path = QDBusObjectPath();
    return QString();
}

void MenuImporter::slotServiceUnregistered(const QString &service)
{
    for (int i = m_registrations.count() - 1; i >= 0; --i) {
        if (m_registrations.at(i).service != service)
            continue;

        const uint id = m_registrations.at(i).windowId;
        removeAt(i);
        emit WindowUnregistered(id);
    }

    m_serviceWatcher->removeWatchedService(service);
}
