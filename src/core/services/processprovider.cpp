#include "processprovider.h"
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QProcess>

ProcessProvider::ProcessProvider(QObject *parent)
    : QObject(parent)
{

}

bool ProcessProvider::startDetached(const QString &exec, QStringList args)
{
    QDBusInterface iface("com.cutefish.Session",
                         "/Session",
                         "com.cutefish.Session", QDBusConnection::sessionBus());

    if (iface.isValid()) {
        iface.asyncCall("launch", exec, args).waitForFinished();
        return true;
    } else {
        QProcess process;
        process.startDetached(exec, args);
    }

    return false;
}
