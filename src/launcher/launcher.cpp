#include "launcher.h"
#include "launcheradaptor.h"
#include "core/services/activity.h"
#include "core/wayland/kwinwaylandinterface.h"
#include "dock/dockwindow.h"
#include "statusbar/statusbar.h"

#include <QGuiApplication>
#include <QPixmapCache>
#include <QQmlContext>
#include <QQuickItem>
#include <QScreen>
#include <QTimer>

Launcher::Launcher(QQmlEngine *engine, bool firstShow)
    : ShellSurface(engine)
    , m_dock(nullptr)
    , m_hideTimer(new QTimer(this))
    , m_showed(false)
    , m_dragging(false)
    , m_topMargin(StatusBar::Height)
    , m_leftMargin(0)
    , m_rightMargin(0)
    , m_bottomMargin(0)
{
    new LauncherAdaptor(this);

    setContextProperty("launcher", this);

    // The launcher paints a translucent full-screen layer on Wayland.  Make
    // sure the scene graph allocates an alpha-capable backing buffer before
    // the surface is first mapped.
    setDefaultAlphaBuffer(true);
    setColor(Qt::transparent);
    setFlags(Qt::FramelessWindowHint);

    // The launcher always covers the primary screen.
    followPrimaryScreen([this] { updateSize(); });

    setSource(QUrl(QStringLiteral("qrc:/launcher/qml/main.qml")));
    setTitle(QStringLiteral("Launcher"));
    configureSurfaceRole(SurfaceRole::Launcher);
    setGeometry(m_screenRect);
    setVisible(true);
    setVisible(firstShow);

    // Let the animation in qml be hidden after the execution is complete
    m_hideTimer->setInterval(200);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [=] { setVisible(false); });

    connect(this, &QQuickWindow::activeChanged, this, &Launcher::onActiveChanged);

    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    connect(kwin, &KWinWaylandInterface::clientAdded, this, [this](quint64 address) {
        focusWaylandWindow(address);
    });
}

int Launcher::topMargin() const
{
    return m_topMargin;
}

int Launcher::leftMargin() const
{
    return m_leftMargin;
}

int Launcher::rightMargin() const
{
    return m_rightMargin;
}

int Launcher::bottomMargin() const
{
    return m_bottomMargin;
}

void Launcher::setDock(DockWindow *dock)
{
    m_dock = dock;
    connect(m_dock, &DockWindow::primaryGeometryChanged, this, &Launcher::updateMargins);
    connect(m_dock, &DockWindow::directionChanged, this, &Launcher::updateMargins);
    updateMargins();
}

bool Launcher::showed()
{
    return m_showed;
}

void Launcher::showWindow()
{
    m_showed = true;
    emit showedChanged();

    // The dock and the status bar stack themselves against the launcher, and
    // in one process there is no WM_CLASS left to tell them apart.
    Activity::self()->setLaunchPad(true);

    // Let KWin see the panels' new stacking behaviour before the launcher is
    // mapped, so it never comes up behind them for a frame.
    KWinWaylandInterface::instance()->roundtrip();

    if (!m_screenRect.isEmpty())
        setGeometry(m_screenRect);
    setVisible(true);
    setSurfacePosition(m_screenRect.topLeft());
    requestActivate();

    // Qt's Wayland requestActivate() has no activation token when the request
    // originates from our D-Bus shortcut. Ask KWin to focus this surface after
    // the compositor has seen it.
    QTimer::singleShot(50, this, [this] {
        focusWaylandWindow();
    });
}

void Launcher::focusWaylandWindow(quint64 address)
{
    if (!m_showed)
        return;

    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    if (address == 0) {
        // The surface may not have been reported yet when showWindow() runs.
        // Refresh once here; future openings can use the cached client.
        kwin->refresh();
        const qint64 pid = QCoreApplication::applicationPid();
        address = kwin->windowForPid(pid, QStringLiteral("Launcher"));
    }

    if (address != 0)
        kwin->focusWindow(address);
}

void Launcher::show()
{
    showWindow();
}

void Launcher::hide()
{
    hideWindow();
}

void Launcher::hideWindow()
{
    m_showed = false;
    m_dragging = false;
    emit showedChanged();
    setVisible(false);

    Activity::self()->setLaunchPad(false);
}

void Launcher::toggle()
{
    // On Wayland a layer-surface can remain mapped while its visibility
    // change is being processed by the compositor.  m_showed is our logical
    // state and must be used for a reliable toggle.
    m_showed ? Launcher::hideWindow() : Launcher::showWindow();
}

bool Launcher::dockAvailable()
{
    return m_dock != nullptr;
}

bool Launcher::isPinedDock(const QString &desktop)
{
    return m_dock && m_dock->pinned(desktop);
}

void Launcher::sendToDock(const QString &desktop)
{
    if (m_dock)
        m_dock->add(desktop);
}

void Launcher::removeFromDock(const QString &desktop)
{
    if (m_dock)
        m_dock->remove(desktop);
}

void Launcher::setDragging(bool dragging)
{
    m_dragging = dragging;
}

void Launcher::clearPixmapCache()
{
    QPixmapCache::clear();
}

QRect Launcher::screenRect()
{
    return m_screenRect;
}

void Launcher::updateMargins()
{
    // The status bar and the dock draw on top of the launcher.
    m_topMargin = StatusBar::Height;
    m_leftMargin = 0;
    m_rightMargin = 0;
    m_bottomMargin = 0;

    if (m_dock) {
        const QRect dockGeometry = m_dock->primaryGeometry();
        const int dockDirection = m_dock->direction();

        if (dockDirection == 0) {
            m_leftMargin = dockGeometry.width();
        } else if (dockDirection == 1) {
            m_bottomMargin = dockGeometry.height();
        } else if (dockDirection == 2) {
            m_rightMargin = dockGeometry.width();
        }
    }

    emit marginsChanged();
}

void Launcher::updateSize()
{
    if (!screen())
        return;

    if (m_screenRect != screen()->geometry()) {
        m_screenRect = screen()->geometry();
        setGeometry(m_screenRect);
        setSurfacePosition(m_screenRect.topLeft());
        emit screenRectChanged();
    }
}

void Launcher::resizeEvent(QResizeEvent *e)
{
    // The launcher's QML root has no size of its own, so it has to follow the
    // window the way QQuickView's SizeRootObjectToView used to make it.
    ShellSurface::resizeEvent(e);

    // KWin advertises the work area as the toplevel's configure bounds, so the
    // first configure after a map shrinks the launcher by the panels' struts.
    // Claiming the screen back takes a configure round trip, and a frame
    // committed in the meantime is the visible jump: keep the contents hidden
    // until the window really covers the screen.
    const bool coversScreen = m_screenRect.isEmpty() || e->size() == m_screenRect.size();
    if (QQuickItem *root = rootItem())
        root->setVisible(coversScreen);

    if (!coversScreen) {
        setGeometry(m_screenRect);

        // Backstop: an invisible launcher is worse than a mis-sized one.
        QTimer::singleShot(500, this, [this] {
            if (rootItem())
                rootItem()->setVisible(true);
        });
    }

    // The window manager forces the size.
    e->ignore();
}

void Launcher::onActiveChanged()
{
    if (isActive() || m_dragging)
        return;

    // A context menu is a separate Qt::Popup window. Opening it makes this
    // window inactive, but the launcher must remain visible while the menu is
    // open.
    QTimer::singleShot(0, this, [this] {
        if (isActive())
            return;

        if (m_dragging)
            return;

        for (QWindow *window : QGuiApplication::allWindows()) {
            if (window != this && window->isVisible()
                    && window->flags().testFlag(Qt::Popup)) {
                return;
            }
        }

        hideWindow();
    });
}
