#ifndef SHELLSURFACE_H
#define SHELLSURFACE_H

#include <QQuickWindow>
#include <QUrl>

class QQmlComponent;
class QQmlContext;
class QQmlEngine;

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

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void resizeRootItem();

    QQmlEngine *m_engine;
    QQmlContext *m_context;
    QQmlComponent *m_component;
    QQuickItem *m_rootItem;
    QUrl m_source;
};

#endif // SHELLSURFACE_H
