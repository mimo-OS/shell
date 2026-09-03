#ifndef CUTEFISH_SHELLSURFACE_H
#define CUTEFISH_SHELLSURFACE_H

#include <QPoint>
#include <QQuickWindow>
#include <QUrl>

#include <functional>

class PlasmaSurface;
class QQmlComponent;
class QQmlContext;
class QQmlEngine;
class QScreen;

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
    //! What the compositor should make of this surface.
    enum class SurfaceRole {
        Desktop,
        Panel,
        Dock,
        Launcher,
    };

    //! What a Panel or Dock surface does about other windows. Ignored for the
    //! other roles.
    enum class PanelBehavior {
        //! Stays above ordinary windows. The reserved work area itself comes
        //! from StrutSpacer, not from here.
        StayAbove,
        //! Lets windows be stacked over the panel while the launcher is up.
        AllowCovering,
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
    void setPanelBehavior(PanelBehavior behavior);

    //! Where the compositor should put this surface, in global coordinates.
    //! See PlasmaSurface::setPosition().
    void setSurfacePosition(const QPoint &position, bool requestRepaint = true);

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
     * keeps showing the panel at the size that buffer implies.
     */
    void watchScreen(QScreen *screen, const std::function<void()> &update);

    /**
     * Moves the window onto the primary screen, follows it from then on, and
     * runs @p update once now and after every change of screen or of that
     * screen's metrics.
     */
    void followPrimaryScreen(const std::function<void()> &update);

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void resizeRootItem();

    QQmlEngine *m_engine;
    QQmlContext *m_context;
    PlasmaSurface *m_plasmaSurface;
    QList<QMetaObject::Connection> m_screenConnections;
    QMetaObject::Connection m_primaryScreenConnection;
    std::function<void()> m_screenUpdate;
    QQmlComponent *m_component;
    QQuickItem *m_rootItem;
    QUrl m_source;
    QPoint m_surfacePosition;
};

#endif // CUTEFISH_SHELLSURFACE_H
