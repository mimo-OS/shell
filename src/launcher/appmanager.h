#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCall>

class AppManager : public QObject
{
    Q_OBJECT

public:
    explicit AppManager(QObject *parent = nullptr);

    Q_INVOKABLE void uninstall(const QString &desktopFile);

    Q_INVOKABLE bool isCutefishOS();

private:
    QDBusInterface m_iface;
};

#endif // APPMANAGER_H
