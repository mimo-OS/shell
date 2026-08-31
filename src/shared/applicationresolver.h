#pragma once

#include <QStringList>

class DesktopEntry;

class ApplicationResolver
{
public:
    static ApplicationResolver *instance();

    QStringList commandFromPid(quint32 pid) const;
    DesktopEntry *resolve(const QString &appId, quint32 pid,
                          const QString &windowClass) const;
    QString desktopPathFromMetadata(const QString &appId, quint32 pid = 0,
                                    const QString &windowClass = QString()) const;
};
