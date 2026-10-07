#include "poweractions.h"
#include <QDebug>
#include <QDBusInterface>
#include <QProcess>

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
        const QDBusMessage reply = iface.call(QStringLiteral("powerOff"));
        if (reply.type() != QDBusMessage::ErrorMessage)
            return;
    }
    QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("poweroff")});
}

void PowerActions::logout()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());
    if (iface.isValid()) {
        const QDBusMessage reply = iface.call(QStringLiteral("logout"));
        if (reply.type() != QDBusMessage::ErrorMessage)
            return;
    }
    QProcess::startDetached(QStringLiteral("loginctl"), {QStringLiteral("terminate-session"), QStringLiteral("self")});
}

void PowerActions::reboot()
{
    QDBusInterface iface(s_dbusName, s_pathName, s_interfaceName, QDBusConnection::sessionBus());
    if (iface.isValid()) {
        const QDBusMessage reply = iface.call(QStringLiteral("reboot"));
        if (reply.type() != QDBusMessage::ErrorMessage)
            return;
    }
    QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("reboot")});
}

void PowerActions::lockScreen()
{
    QProcess::startDetached(QStringLiteral("cutefish-screenlocker"), QStringList());
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
