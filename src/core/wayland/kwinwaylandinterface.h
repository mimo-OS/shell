#ifndef KWINWAYLANDINTERFACE_H
#define KWINWAYLANDINTERFACE_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QRect>
#include <QString>

struct zwlr_layer_shell_v1;

namespace KWayland
{
namespace Client
{
class Compositor;
class ConnectionThread;
class PlasmaShell;
class PlasmaVirtualDesktopManagement;
class PlasmaVirtualDesktop;
class PlasmaWindow;
class PlasmaWindowManagement;
class Registry;
class ShmPool;
class Surface;
}
}

struct KWinWaylandClient
{
    quint64 id = 0;
    QString className;
    QString initialClass;
    QString resourceName;
    QString title;
    QString initialTitle;
    QString workspaceName;
    QString appMenuServiceName;
    QString appMenuObjectPath;
    QStringList plasmaVirtualDesktops;
    bool onAllDesktops = true;
    qint64 pid = 0;
    QRect geometry;
    bool mapped = true;
    bool hidden = false;
    bool skipTaskbar = false;
    bool skipSwitcher = false;
    bool maximized = false;
    bool floating = true;
    bool pinned = false;
    int fullscreen = 0;
    int fullscreenClient = 0;

    bool operator==(const KWinWaylandClient &other) const
    {
        return id == other.id && className == other.className
            && initialClass == other.initialClass && title == other.title
            && resourceName == other.resourceName
            && initialTitle == other.initialTitle && pid == other.pid
            && appMenuServiceName == other.appMenuServiceName
            && appMenuObjectPath == other.appMenuObjectPath
            && plasmaVirtualDesktops == other.plasmaVirtualDesktops
            && onAllDesktops == other.onAllDesktops
            && geometry == other.geometry
            && mapped == other.mapped && hidden == other.hidden
            && skipTaskbar == other.skipTaskbar
            && skipSwitcher == other.skipSwitcher
            && maximized == other.maximized
            && fullscreen == other.fullscreen
            && fullscreenClient == other.fullscreenClient;
    }
};

class KWinWaylandInterface : public QObject
{
    Q_OBJECT

public:
    static KWinWaylandInterface *instance();
    explicit KWinWaylandInterface(QObject *parent = nullptr);

    bool available() const;

    //! The shell positions its own panels through org_kde_plasma_shell rather
    //! than wlr-layer-shell: layer-shell is a per-process QPA shell
    //! integration, and it would turn every menu, tooltip and dialog of this
    //! process into a layer surface as well.
    KWayland::Client::PlasmaShell *plasmaShell() const;

    //! Pieces StrutSpacer needs to build a bare wlr-layer-shell surface of its
    //! own, without the LayerShellQt QPA integration.
    KWayland::Client::Compositor *compositor() const;
    KWayland::Client::ShmPool *shmPool() const;
    zwlr_layer_shell_v1 *layerShell() const;
    QList<KWinWaylandClient> clients() const;
    KWinWaylandClient client(quint64 id) const;

    //! The application id the compositor reports: className, or initialClass
    //! while the window is still coming up.
    static QString appId(const KWinWaylandClient &client);

    //! Whether @p id is a window the task list should show: mapped, and
    //! neither one of the shell's own windows nor a compositor helper.
    bool isAcceptableWindow(quint64 id) const;
    quint64 activeWindow() const;
    quint64 windowForPid(qint64 pid, const QString &title = {}) const;
    bool hasWindowTouching(const QRect &rect) const;
    bool hasFullscreenWindow(const QRect &screenRect = QRect()) const;
    bool isOnCurrentDesktop(const KWinWaylandClient &client) const;

    void refresh();

    //! Blocks until the compositor has processed everything sent so far. The
    //! launcher needs it to be sure KWin has taken the panels' reserved area
    //! back before its window is mapped.
    void roundtrip();
    void focusWindow(quint64 id);
    void minimizeWindow(quint64 id);
    void restoreWindow(quint64 id);
    void closeWindow(quint64 id);
    void toggleMaximize(quint64 id);
    void startMove(quint64 id);
    void setMinimizedGeometry(quint64 id, KWayland::Client::Surface *panel,
                              const QRect &geometry);

signals:
    void layerShellReady();
    void plasmaShellReady();
    void windowManagementReady();
    void clientAdded(quint64 id);
    void clientRemoved(quint64 id);
    void clientChanged(quint64 id);
    void activeWindowChanged(quint64 id);

private:
    static quint64 idFor(KWayland::Client::PlasmaWindow *window);

    void setupPlasmaShell(quint32 name, quint32 version);
    void setupInterface(const QByteArray &interface, quint32 name, quint32 version);
    void setupWindowManagement(quint32 name, quint32 version);
    void setupVirtualDesktopManagement(quint32 name, quint32 version);
    void addWindow(KWayland::Client::PlasmaWindow *window);
    void updateWindow(KWayland::Client::PlasmaWindow *window);
    void removeWindow(quint64 id);
    void updateActiveWindow();

    KWinWaylandClient toClient(KWayland::Client::PlasmaWindow *window) const;
    KWayland::Client::PlasmaWindow *window(quint64 id) const;

    KWayland::Client::ConnectionThread *m_connection = nullptr;
    KWayland::Client::Registry *m_registry = nullptr;
    KWayland::Client::Compositor *m_compositor = nullptr;
    KWayland::Client::ShmPool *m_shmPool = nullptr;
    zwlr_layer_shell_v1 *m_layerShell = nullptr;
    KWayland::Client::PlasmaShell *m_plasmaShell = nullptr;
    KWayland::Client::PlasmaWindowManagement *m_windowManagement = nullptr;
    KWayland::Client::PlasmaVirtualDesktopManagement *m_virtualDesktopManagement = nullptr;
    QString m_activeDesktopId;
    QHash<quint64, KWinWaylandClient> m_clients;
    QHash<quint64, KWayland::Client::PlasmaWindow *> m_windows;
    quint64 m_activeWindow = 0;
    bool m_available = false;
};

#endif // KWINWAYLANDINTERFACE_H
