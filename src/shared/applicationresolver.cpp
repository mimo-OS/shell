#include "applicationresolver.h"

#include "applicationregistry.h"

#include <QFile>
#include <QFileInfo>

namespace {

QStringList processCommand(quint32 pid)
{
    QFile file(QStringLiteral("/proc/%1/cmdline").arg(pid));
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QList<QByteArray> arguments = file.readAll().split('\0');
    if (arguments.isEmpty() || arguments.first().isEmpty())
        return {};

    const QString program = QString::fromLocal8Bit(arguments.first());
    return {program, QFileInfo(program).fileName()};
}

} // namespace

ApplicationResolver *ApplicationResolver::instance()
{
    static ApplicationResolver resolver;
    return &resolver;
}

QStringList ApplicationResolver::commandFromPid(quint32 pid) const
{
    return processCommand(pid);
}

DesktopEntry *ApplicationResolver::resolve(const QString &appId, quint32 pid,
                                           const QString &windowClass) const
{
    const QStringList command = processCommand(pid);
    return ApplicationRegistry::instance()->resolveWindow(
        appId,
        windowClass,
        command.value(0),
        command.value(1));
}

QString ApplicationResolver::desktopPathFromMetadata(const QString &appId,
                                                     quint32 pid,
                                                     const QString &windowClass) const
{
    if (DesktopEntry *entry = resolve(appId, pid, windowClass))
        return entry->path();
    return {};
}
