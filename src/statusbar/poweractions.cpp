#include "poweractions.h"
#include <QDebug>
#include <QDBusInterface>

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
    if (!iface.isValid()) {
        qWarning() << "Unable to request shutdown: session service is unavailable";
        return;
    }

    const QDBusMessage reply = iface.call(QStringLiteral("powerOff"));
    if (reply.type() == QDBusMessage::ErrorMessage)
        qWarning() << "Unable to request shutdown:" << reply.errorName() << reply.errorMessage();
}

void PowerActions::logout()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "Unable to request logout: session service is unavailable";
        return;
    }

    iface.call(QStringLiteral("logout"));
}

void PowerActions::reboot()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "Unable to request reboot: session service is unavailable";
        return;
    }

    const QDBusMessage reply = iface.call(QStringLiteral("reboot"));
    if (reply.type() == QDBusMessage::ErrorMessage)
        qWarning() << "Unable to request reboot:" << reply.errorName() << reply.errorMessage();
}

void PowerActions::lockScreen()
{
    QDBusInterface iface(s_screenSaverService,
                         s_screenSaverPath,
                         s_screenSaverInterface,
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.asyncCall(QStringLiteral("Lock"));
    } else {
        qWarning() << "Unable to request screen lock: screen saver service is unavailable";
    }
}

void PowerActions::suspend()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());

    if (!iface.isValid()) {
        qWarning() << "Unable to request suspend: session service is unavailable";
        return;
    }

    const QDBusMessage reply = iface.call(QStringLiteral("suspend"));
    if (reply.type() == QDBusMessage::ErrorMessage)
        qWarning() << "Unable to request suspend:" << reply.errorName() << reply.errorMessage();
}
