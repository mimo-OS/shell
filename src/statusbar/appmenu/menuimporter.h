#ifndef MENUIMPORTER_H
#define MENUIMPORTER_H

// Qt
#include <QDBusArgument>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>

class QDBusObjectPath;
class QDBusServiceWatcher;

class MenuImporter : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    explicit MenuImporter(QObject *);
    ~MenuImporter() override;

    //! The status bar reads the registrar from QML, which has no handle on the
    //! AppMenu object that owns this importer.
    static MenuImporter *instance();

    bool connectToBus();

    bool serviceExist(uint id)
    {
        return m_menuServices.contains(id);
    }
    QString serviceForWindow(uint id)
    {
        return m_menuServices.value(id);
    }

    bool pathExist(uint id)
    {
        return m_menuPaths.contains(id);
    }
    QString pathForWindow(uint id)
    {
        return m_menuPaths.value(id).path();
    }

    QList<uint> ids()
    {
        return m_menuServices.keys();
    }

    //! Wayland has no window id a client and the compositor agree on, so the
    //! menu a window owns is found through the pid of the process that
    //! registered it.
    uint windowForPid(uint pid) const;
    QString serviceForPid(uint pid) const
    {
        return m_menuServices.value(windowForPid(pid));
    }
    QString pathForPid(uint pid) const
    {
        return m_menuPaths.value(windowForPid(pid)).path();
    }

Q_SIGNALS:
    void WindowRegistered(uint id, const QString &service, const QDBusObjectPath &);
    void WindowUnregistered(uint id);

public Q_SLOTS:
    Q_NOREPLY void RegisterWindow(uint id, const QDBusObjectPath &path);
    Q_NOREPLY void UnregisterWindow(uint id);
    QString GetMenuForWindow(uint id, QDBusObjectPath &path);

private Q_SLOTS:
    void slotServiceUnregistered(const QString &service);

private:
    QDBusServiceWatcher *m_serviceWatcher;
    QHash<uint, QString> m_menuServices;
    QHash<uint, QDBusObjectPath> m_menuPaths;
    QHash<uint, uint> m_windowPids;
};

#endif /* MENUIMPORTER_H */
