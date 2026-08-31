#include "utils.h"

#include "applicationregistry.h"
#include "applicationresolver.h"

static Utils *INSTANCE = nullptr;

Utils *Utils::instance()
{
    if (!INSTANCE)
        INSTANCE = new Utils;

    return INSTANCE;
}

Utils::Utils(QObject *parent)
    : QObject(parent)
{
}

QStringList Utils::commandFromPid(quint32 pid)
{
    return ApplicationResolver::instance()->commandFromPid(pid);
}

QString Utils::desktopPathFromMetadata(const QString &appId, quint32 pid,
                                      const QString &windowClassName)
{
    return ApplicationResolver::instance()->desktopPathFromMetadata(appId, pid,
                                                                     windowClassName);
}

QMap<QString, QString> Utils::readInfoFromDesktop(const QString &desktopFile)
{
    QMap<QString, QString> info;
    DesktopEntry *entry = ApplicationRegistry::instance()->byPath(desktopFile);
    if (!entry)
        return info;

    info.insert(QStringLiteral("Icon"), entry->icon());
    info.insert(QStringLiteral("Name"), entry->name());
    info.insert(QStringLiteral("Exec"), entry->exec());
    info.insert(QStringLiteral("Id"), entry->id());
    return info;
}
