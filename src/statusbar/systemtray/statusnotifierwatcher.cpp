#include "statusnotifierwatcher.h"
#include "statusnotifieritem_interface.h"
#include "statusnotifierwatcheradaptor.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDebug>

StatusNotifierWatcher::StatusNotifierWatcher(QObject *parent)
    : QObject(parent)
{
    new StatusNotifierWatcherAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/StatusNotifierWatcher"), this);
    dbus.registerService(QStringLiteral("org.kde.StatusNotifierWatcher"));

    m_serviceWatcher = new QDBusServiceWatcher(this);
    m_serviceWatcher->setConnection(dbus);
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &StatusNotifierWatcher::serviceUnregistered);
}

StatusNotifierWatcher::~StatusNotifierWatcher()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService(QStringLiteral("org.kde.StatusNotifierWatcher"));
}

QStringList StatusNotifierWatcher::RegisteredStatusNotifierItems() const
{
    return m_registeredServices;
}

bool StatusNotifierWatcher::IsStatusNotifierHostRegistered() const
{
    return !m_statusNotifierHostServices.isEmpty();
}

void StatusNotifierWatcher::RegisterStatusNotifierItem(const QString &serviceOrPath)
{
    QString service;
    QString path;
    if (serviceOrPath.startsWith(QLatin1Char('/'))) {
        service = message().service();
        path = serviceOrPath;
    } else {
        int slash = serviceOrPath.indexOf(QLatin1Char('/'));
        if (slash != -1) {
            service = serviceOrPath.left(slash);
            path = serviceOrPath.mid(slash);
        } else {
            service = serviceOrPath;
            path = QStringLiteral("/StatusNotifierItem");
        }
    }
    if (service.isEmpty()) {
        service = message().service();
    }
    if (path.isEmpty()) {
        path = QStringLiteral("/StatusNotifierItem");
    }

    QString notifierItemId = service + path;
    if (m_registeredServices.contains(notifierItemId)) {
        return;
    }

    m_serviceWatcher->addWatchedService(service);

    bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered(service).value();
    if (!registered && !message().service().isEmpty()) {
        registered = QDBusConnection::sessionBus().interface()->isServiceRegistered(message().service()).value();
        if (registered && !service.startsWith(QLatin1Char(':'))) {
            m_serviceWatcher->addWatchedService(message().service());
        }
    }

    if (registered) {
        org::kde::StatusNotifierItem trayclient(service, path, QDBusConnection::sessionBus());
        if (trayclient.isValid()) {
            qDebug() << "Registering" << notifierItemId << "to system tray";
            m_registeredServices.append(notifierItemId);
            emit StatusNotifierItemRegistered(notifierItemId);
            return;
        }
    }

    m_serviceWatcher->removeWatchedService(service);
}

void StatusNotifierWatcher::RegisterStatusNotifierHost(const QString &service)
{
    QString hostService = service.isEmpty() ? message().service() : service;
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(hostService).value()
        && !m_statusNotifierHostServices.contains(hostService)) {
        qDebug() << "Registering" << hostService << "as system tray host";

        m_statusNotifierHostServices.insert(hostService);
        m_serviceWatcher->addWatchedService(hostService);
        emit StatusNotifierHostRegistered();
    }
}

void StatusNotifierWatcher::serviceUnregistered(const QString &name)
{
    qDebug() << "Service " << name << "unregistered";
    m_serviceWatcher->removeWatchedService(name);

    QString match = name + QLatin1Char('/');
    QStringList::Iterator it = m_registeredServices.begin();
    while (it != m_registeredServices.end()) {
        if (it->startsWith(match)) {
            QString name = *it;
            it = m_registeredServices.erase(it);
            emit StatusNotifierItemUnregistered(name);
        } else {
            ++it;
        }
    }

    if (m_statusNotifierHostServices.contains(name)) {
        m_statusNotifierHostServices.remove(name);
        emit StatusNotifierHostUnregistered();
    }
}
