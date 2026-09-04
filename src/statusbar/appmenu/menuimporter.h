#ifndef MENUIMPORTER_H
#define MENUIMPORTER_H

// Qt
#include <QDBusArgument>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QList>
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

    //! Wayland has no window id a client and the compositor agree on, so the
    //! menu a window owns is found through the pid of the process that
    //! registered it.
    QString serviceForPid(uint pid) const;
    QString pathForPid(uint pid) const;

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
    //! Qt hands the registrar a per-process window counter on Wayland, so the
    //! same id arrives from several applications at once; only the pair with
    //! the exporter's unique bus name identifies a menu.
    struct Registration {
        uint windowId = 0;
        QString service;
        QDBusObjectPath path;
        uint pid = 0;
    };

    const Registration *newestFor(uint pid) const;
    void removeAt(int index);

    QDBusServiceWatcher *m_serviceWatcher;
    // Ordered oldest first; the newest registration of a process wins.
    QList<Registration> m_registrations;
};

#endif /* MENUIMPORTER_H */
