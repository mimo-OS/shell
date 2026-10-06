#include "processprovider.h"
#include "applicationlauncher.h"
#include <QProcess>

ProcessProvider::ProcessProvider(QObject *parent)
    : QObject(parent)
{

}

bool ProcessProvider::startDetached(const QString &exec, QStringList args)
{
    if (ApplicationLauncher::startDetached(QStringList{exec} + args))
        return true;

    if (QProcess::startDetached(exec, args))
        return true;

    if (exec == QStringLiteral("cutefish-settings")) {
        if (QProcess::startDetached(QStringLiteral("systemsettings"), args))
            return true;
        if (QProcess::startDetached(QStringLiteral("gnome-control-center"), args))
            return true;
    }
    return false;
}
