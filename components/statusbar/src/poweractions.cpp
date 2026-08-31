#include "poweractions.h"
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QApplication>

const static QString s_dbusName = "com.cutefish.Session";
const static QString s_pathName = "/Session";
const static QString s_interfaceName = "com.cutefish.Session";

const static QString s_screenSaverService = "org.freedesktop.ScreenSaver";
const static QString s_screenSaverPath = "/ScreenSaver";
const static QString s_screenSaverInterface = "org.freedesktop.ScreenSaver";

PowerActions::PowerActions(QObject *parent)
    : QObject(parent)
{

}

void PowerActions::shutdown()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.call("powerOff");
    }
}

void PowerActions::logout()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.call("logout");
    }
}

void PowerActions::reboot()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.call("reboot");
    }
}

void PowerActions::lockScreen()
{
    QDBusInterface iface(s_screenSaverService,
                         s_screenSaverPath,
                         s_screenSaverInterface,
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.asyncCall(QStringLiteral("Lock"));
    }
}

void PowerActions::suspend()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());

    if (iface.isValid()) {
        iface.call("suspend");
    }
}
