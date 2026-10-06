#include "core/window/shellsurface.h"
#include "core/window/plasmasurface.h"

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>
#include <QDebug>

ShellSurface::ShellSurface(QQmlEngine *engine, QWindow *parent)
    : QQuickWindow(parent)
    , m_engine(engine)
    , m_context(new QQmlContext(engine->rootContext(), this))
    , m_plasmaSurface(new PlasmaSurface(this))
    , m_component(nullptr)
    , m_rootItem(nullptr)
{
}

ShellSurface::~ShellSurface()
{
    delete m_rootItem;
    m_rootItem = nullptr;
}

QQmlEngine *ShellSurface::engine() const
{
    return m_engine;
}

QQmlContext *ShellSurface::context() const
{
    return m_context;
}

QUrl ShellSurface::source() const
{
    return m_source;
}

QQuickItem *ShellSurface::rootItem() const
{
    return m_rootItem;
}

void ShellSurface::setContextProperty(const QString &name, QObject *value)
{
    m_context->setContextProperty(name, value);
}

void ShellSurface::setContextProperty(const QString &name, const QVariant &value)
{
    m_context->setContextProperty(name, value);
}

void ShellSurface::configureSurfaceRole(SurfaceRole role)
{
    switch (role) {
    case SurfaceRole::Desktop:
        m_plasmaSurface->setRole(PlasmaSurface::Role::Desktop);
        break;
    case SurfaceRole::Panel:
    case SurfaceRole::Dock:
        // Panels must not take the keyboard focus. A dock that steals focus
        // makes the launcher hide itself on the very click that is meant to
        // toggle it, so it can never be closed from its own dock button.
        m_plasmaSurface->setPanelTakesFocus(false);
        m_plasmaSurface->setRole(PlasmaSurface::Role::Panel);
        break;
    case SurfaceRole::Launcher:
        // An ordinary window keeps the launcher in the normal layer, below the
        // dock and the status bar, which both sit in KWin's "above" layer as
        // panels. A panel role would put the launcher in the same layer as the
        // dock, where being the active window would raise it over the dock
        // instead of under it.
        m_plasmaSurface->setRole(PlasmaSurface::Role::Normal);
        break;
    }
}

void ShellSurface::setPanelBehavior(PanelBehavior behavior)
{
    // AlwaysVisible would be the natural choice, but KWin uses it only for
    // stacking: work area comes from StrutSpacer either way.
    m_plasmaSurface->setPanelBehavior(
        behavior == PanelBehavior::AllowCovering
            ? PlasmaSurface::PanelBehavior::WindowsCanCover
            : PlasmaSurface::PanelBehavior::WindowsGoBelow);
}

void ShellSurface::setSurfacePosition(const QPoint &position, bool requestRepaint)
{
    const bool moved = m_surfacePosition != position;
    m_surfacePosition = position;
    m_plasmaSurface->setPosition(position);

    // KWin applies the position with the next commit of the surface, and a
    // panel that only moves has nothing new to draw: the dock kept landing
    // where the *previous* screen change had put it, one resolution behind.
    if (moved && requestRepaint)
        requestUpdate();

    if (moved)
        emit surfacePositionChanged();
}

void ShellSurface::watchScreen(QScreen *screen, const std::function<void()> &update)
{
    for (const QMetaObject::Connection &connection : std::as_const(m_screenConnections))
        disconnect(connection);

    m_screenConnections.clear();

    if (!screen)
        return;

    const auto watch = [this, screen, update](auto signal) {
        m_screenConnections << connect(screen, signal, this, [update] {
            if (update)
                update();
        }, Qt::QueuedConnection);
    };

    watch(&QScreen::geometryChanged);
    watch(&QScreen::virtualGeometryChanged);
    // A scale change that comes with a matching mode change leaves the screen
    // the same size in logical pixels; only the pixel density says so.
    watch(&QScreen::physicalDotsPerInchChanged);

    m_screenConnections << connect(screen, &QObject::destroyed, this, [this](QObject *) {
        for (const QMetaObject::Connection &connection : std::as_const(m_screenConnections))
            disconnect(connection);
        m_screenConnections.clear();
    });
}

void ShellSurface::followPrimaryScreen(const std::function<void()> &update)
{
    m_screenUpdate = update;

    if (!m_primaryScreenConnection) {
        m_primaryScreenConnection =
            connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this,
                    [this](QScreen *screen) {
                        setScreen(screen);
                        watchScreen(screen, m_screenUpdate);
                        if (m_screenUpdate)
                            m_screenUpdate();
                    });
    }

    setScreen(qGuiApp->primaryScreen());
    watchScreen(screen(), m_screenUpdate);
    if (m_screenUpdate)
        m_screenUpdate();
}

void ShellSurface::setSource(const QUrl &url)
{
    if (m_source == url && m_rootItem)
        return;

    m_source = url;

    delete m_rootItem;
    m_rootItem = nullptr;
    delete m_component;

    m_component = new QQmlComponent(m_engine, url, this);

    if (m_component->isError()) {
        qWarning() << "ShellSurface: cannot load" << url << m_component->errors();
        return;
    }

    // beginCreate/completeCreate so the root item is parented and sized before
    // its bindings run; otherwise anchors against the window flicker on startup.
    QObject *object = m_component->beginCreate(m_context);
    m_rootItem = qobject_cast<QQuickItem *>(object);

    if (!m_rootItem) {
        qWarning() << "ShellSurface: the root object of" << url << "is not an Item.";
        delete object;
        return;
    }

    m_rootItem->setParentItem(contentItem());
    resizeRootItem();
    m_component->completeCreate();

    if (m_component->isError())
        qWarning() << "ShellSurface:" << url << m_component->errors();
}

void ShellSurface::resizeRootItem()
{
    if (!m_rootItem)
        return;

    m_rootItem->setPosition(QPointF(0, 0));
    m_rootItem->setSize(QSizeF(width(), height()));
}

void ShellSurface::resizeEvent(QResizeEvent *e)
{
    resizeRootItem();
    QQuickWindow::resizeEvent(e);
}
