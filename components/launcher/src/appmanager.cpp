#include "appmanager.h"
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
    if (m_iface.isValid()) {
        m_iface.asyncCall("uninstall", desktopFile);
    }
}

bool AppManager::isCutefishOS()
{
    return QFile::exists("/etc/cutefishos");
}
