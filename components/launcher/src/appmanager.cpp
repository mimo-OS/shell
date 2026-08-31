#include "appmanager.h"
#include "applicationregistry.h"
#include <QFile>

AppManager::AppManager(QObject *parent)
    : QObject(parent)
    , m_iface("com.cutefish.Daemon",
              "/AppManager",
              "com.cutefish.AppManager", QDBusConnection::systemBus())
{

}

void AppManager::uninstall(const QString &desktopFile)
{
    QString path = desktopFile;
    if (DesktopEntry *entry = ApplicationRegistry::instance()->byId(desktopFile))
        path = entry->path();

    if (m_iface.isValid()) {
        m_iface.asyncCall("uninstall", path);
    }
}

bool AppManager::isCutefishOS()
{
    return QFile::exists("/etc/cutefishos");
}
