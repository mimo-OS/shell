#include "shellsurface.h"

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QPlatformSurfaceEvent>
#include <QDebug>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#include "kwinwaylandinterface.h"

using namespace KWayland::Client;

ShellSurface::ShellSurface(QQmlEngine *engine, QWindow *parent)
    : QQuickWindow(parent)
    , m_engine(engine)
    , m_context(new QQmlContext(engine->rootContext(), this))
    , m_component(nullptr)
    , m_rootItem(nullptr)
    , m_plasmaSurface(nullptr)
    , m_wlSurface(nullptr)
    , m_role(SurfaceRole::Panel)
    , m_panelBehavior(PanelBehavior::StayAbove)
    , m_hasRole(false)
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
    m_role = role;
    m_hasRole = true;
    applySurfaceRole();
}

void ShellSurface::setPanelBehavior(PanelBehavior behavior)
{
    if (m_panelBehavior == behavior)
        return;

    m_panelBehavior = behavior;

    if (m_plasmaSurface && (m_role == SurfaceRole::Panel || m_role == SurfaceRole::Dock))
        m_plasmaSurface->setPanelBehavior(plasmaPanelBehavior());
}

void ShellSurface::applySurfaceRole()
{
    if (!m_hasRole)
        return;

    PlasmaShell *shell = KWinWaylandInterface::instance()->plasmaShell();
    if (!shell) {
        qWarning() << "No org_kde_plasma_shell; cannot set the surface role of" << title();
        return;
    }

    // Qt drops the wl_surface whenever the window is hidden, so the
    // plasma-shell surface has to be built again from the current one rather
    // than kept for the lifetime of the window.
    Surface *surface = Surface::fromWindow(this);
    wl_surface *native = surface ? static_cast<wl_surface *>(*surface) : nullptr;
    if (!native)
        return;

    if (native == m_wlSurface && m_plasmaSurface) {
        m_plasmaSurface->setPosition(m_surfacePosition);
        return;
    }

    delete m_plasmaSurface;
    m_wlSurface = native;
    m_plasmaSurface = shell->createSurface(surface, this);
    if (!m_plasmaSurface)
        return;

    switch (m_role) {
    case SurfaceRole::Desktop:
        m_plasmaSurface->setRole(PlasmaShellSurface::Role::Desktop);
        break;
    case SurfaceRole::Panel:
    case SurfaceRole::Dock:
        m_plasmaSurface->setRole(PlasmaShellSurface::Role::Panel);
        m_plasmaSurface->setPanelBehavior(plasmaPanelBehavior());
        // Panels must not take the keyboard focus. A dock that steals focus makes
        // the launcher hide itself on the very click that is meant to toggle
        // it, so it can never be closed from its own dock button.
        m_plasmaSurface->setPanelTakesFocus(false);
        break;
    case SurfaceRole::Launcher:
        // An ordinary window keeps the launcher in the normal layer, below the
        // dock and the status bar, which both sit in KWin's
        // "above" layer as panels. A panel role would put the launcher in the
        // same layer as the dock, where being the active window would raise it
        // over the dock instead of under it.
        m_plasmaSurface->setRole(PlasmaShellSurface::Role::Normal);
        break;
    }

    m_plasmaSurface->setSkipTaskbar(true);
    m_plasmaSurface->setSkipSwitcher(true);
    m_plasmaSurface->setPosition(m_surfacePosition);
}

PlasmaShellSurface::PanelBehavior ShellSurface::plasmaPanelBehavior() const
{
    // AlwaysVisible would be the natural choice, but KWin uses it only for
    // stacking: work area comes from StrutSpacer either way.
    return m_panelBehavior == PanelBehavior::AllowCovering
        ? PlasmaShellSurface::PanelBehavior::WindowsCanCover
        : PlasmaShellSurface::PanelBehavior::WindowsGoBelow;
}

void ShellSurface::setSurfacePosition(const QPoint &position)
{
    m_surfacePosition = position;

    if (m_plasmaSurface)
        m_plasmaSurface->setPosition(position);
}

bool ShellSurface::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        switch (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            // The wl_surface exists from here on, and nothing has been
            // committed on it yet. Claiming the role and the position now
            // rather than on the first expose is what keeps KWin from running
            // its placement pass over the surface: a window that already knows
            // where it goes is left alone, while one that does not gets shrunk
            // to the work area first and has to be resized back, which the
            // launcher went through visibly every time it opened.
            applySurfaceRole();
            break;
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            delete m_plasmaSurface;
            m_plasmaSurface = nullptr;
            m_wlSurface = nullptr;
            break;
        }
    }

    return QQuickWindow::event(e);
}

void ShellSurface::exposeEvent(QExposeEvent *e)
{
    QQuickWindow::exposeEvent(e);

    // A newly exposed window has a fresh wl_surface; re-apply the role to it.
    if (isExposed())
        applySurfaceRole();
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
