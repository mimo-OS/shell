#include "processprovider.h"
#include "applicationlauncher.h"

ProcessProvider::ProcessProvider(QObject *parent)
    : QObject(parent)
{

}

bool ProcessProvider::startDetached(const QString &exec, QStringList args)
{
    return ApplicationLauncher::startDetached(QStringList{exec} + args);
}
