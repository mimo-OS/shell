#include "appmenudbus.h"
#include "appmenuadaptor.h"
#include "kdbusimporter.h"

#include <QApplication>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

static const char *DBUS_SERVICE = "com.cutefish.cappmenu";
static const char *DBUS_OBJECT_PATH = "/CAppMenu";

AppmenuDBus::AppmenuDBus(QObject *parent)
    : QObject(parent)
{
}

AppmenuDBus::~AppmenuDBus()
{
}

bool AppmenuDBus::connectToBus(const QString &service, const QString &path)
{
    m_service = service.isEmpty() ? DBUS_SERVICE : service;
    QString newPath = path.isEmpty() ? DBUS_OBJECT_PATH : path;

    if (!QDBusConnection::sessionBus().registerService(m_service)) {
        return false;
    }
    new AppmenuAdaptor(this);
    QDBusConnection::sessionBus().registerObject(newPath, this);

    return true;
}

void AppmenuDBus::showMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId)
{
    emit appShowMenu(x, y, serviceName, menuObjectPath, actionId);
}

void AppmenuDBus::reconfigure()
{
    emit reconfigured();
}
