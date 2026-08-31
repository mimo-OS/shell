#ifndef SHELLSURFACE_H
#define SHELLSURFACE_H

#include <QPoint>
#include <QQuickWindow>
#include <QSize>
#include <QUrl>

#include <functional>

struct wl_surface;

class QQmlComponent;
class QQmlContext;
class QQmlEngine;
class QScreen;

#include <KWayland/Client/plasmashell.h>

/**
 * A shell window that draws QML from the shell's single, shared QQmlEngine.
 *
 * This is what QQuickView would be if it did not insist on owning an engine.
 * Every shell component (status bar, dock, launcher, desktop) is one of these,
 * so the whole shell parses QML, loads plugins and caches images exactly once.
 * That also sidesteps Qt 6 calling QQmlExtensionPlugin::initializeEngine() only
 * for the first engine, which used to leave FishUI's "icontheme" image provider
 * missing in every window after the first.
 *
 * Each surface gets its own QQmlContext, a child of the engine's root context,
 * so components can expose context properties under whatever name they like
 * without colliding with each other.
 *
 * The root item is always resized to the window, matching the
 * QQuickView::SizeRootObjectToView behaviour the components were written for.
 */
class ShellSurface : public QQuickWindow
{
    Q_OBJECT

public:
    //! What the compositor should make of this surface. Mapped onto
    //! org_kde_plasma_shell roles used by the Wayland compositor.
    enum class SurfaceRole {
        Desktop,
        Panel,
        Dock,
        Launcher,
    };

    explicit ShellSurface(QQmlEngine *engine, QWindow *parent = nullptr);
    ~ShellSurface() override;

    QQmlEngine *engine() const;
    QQmlContext *context() const;

    /** Instantiates @p url as this window's contents. */
    void setSource(const QUrl &url);
    QUrl source() const;

    QQuickItem *rootItem() const;

    void setContextProperty(const QString &name, QObject *value);
    void setContextProperty(const QString &name, const QVariant &value);

    void configureSurfaceRole(SurfaceRole role);
    //! What a Role::Panel surface does about other windows. Ignored for the
        //! other roles.
    enum class PanelBehavior {
        //! Stays above ordinary windows. The reserved work area itself comes
        //! from StrutSpacer, not from here.
        StayAbove,
        //! Lets windows be stacked over the panel while the launcher is up.
        AllowCovering,
    };
    void setPanelBehavior(PanelBehavior behavior);
    //! Where the compositor should put this surface. It cannot be read back
    //! from QWindow::geometry(): Qt's Wayland plugin resets a toplevel's
    //! position to (0, 0) because plain xdg-shell has no global coordinates.
    void setSurfacePosition(const QPoint &position);

    /**
     * Runs @p update whenever the metrics of @p screen change, and until the
     * next call to watchScreen().
     *
     * The call is deferred to the next pass of the event loop on purpose. Qt's
     * Wayland plugin emits QScreen::geometryChanged from the middle of its own
     * output handling and then goes on to resize every window on that screen
     * itself, to keep them the same size in physical pixels. A panel that
     * applies its new geometry straight from the signal handler is therefore
     * overwritten again, and because Qt sees no change afterwards the window is
     * never repainted: the surface keeps the buffer it had, and the compositor
     * keeps showing the panel at the size that buffer implies. That is what
     * left the status bar the width of the old resolution, and the desktop
     * covering a quarter of the screen, after every scale change.
     */
    void watchScreen(QScreen *screen, const std::function<void()> &update);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void exposeEvent(QExposeEvent *e) override;
    bool event(QEvent *e) override;

private:
    void applySurfaceRole();
    KWayland::Client::PlasmaShellSurface::PanelBehavior plasmaPanelBehavior() const;

private:
    void resizeRootItem();

    QQmlEngine *m_engine;
    QQmlContext *m_context;
    QList<QMetaObject::Connection> m_screenConnections;
    QQmlComponent *m_component;
    QQuickItem *m_rootItem;
    QUrl m_source;
    KWayland::Client::PlasmaShellSurface *m_plasmaSurface;
    wl_surface *m_wlSurface;
    QPoint m_surfacePosition;
    SurfaceRole m_role;
    PanelBehavior m_panelBehavior;
    bool m_hasRole;
};

#endif // SHELLSURFACE_H
