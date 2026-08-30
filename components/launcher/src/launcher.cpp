#include "launcher.h"
#include "launcheradaptor.h"
#include "activity.h"
#include "kwinwaylandinterface.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QPixmapCache>
#include <QQmlContext>
#include <QScreen>
#include <QTimer>

#include <KX11Extras>
#include <KWindowSystem>

Launcher::Launcher(QQmlEngine *engine, bool firstShow)
    : ShellSurface(engine)
    , m_dockInterface("com.cutefish.Dock",
                    "/Dock",
                    "com.cutefish.Dock", QDBusConnection::sessionBus())
    , m_hideTimer(new QTimer)
    , m_showed(false)
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
    onGeometryChanged();

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

    if (m_dockInterface.isValid() && !m_dockInterface.lastError().isValid()) {
        updateMargins();
        connect(&m_dockInterface, SIGNAL(primaryGeometryChanged()), this, SLOT(updateMargins()));
        connect(&m_dockInterface, SIGNAL(directionChanged()), this, SLOT(updateMargins()));
    } else {
        QDBusServiceWatcher *watcher = new QDBusServiceWatcher("com.cutefish.Dock",
                                                               QDBusConnection::sessionBus(),
                                                               QDBusServiceWatcher::WatchForUnregistration,
                                                               this);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [=] {
            updateMargins();
            connect(&m_dockInterface, SIGNAL(primaryGeometryChanged()), this, SLOT(updateMargins()));
            connect(&m_dockInterface, SIGNAL(directionChanged()), this, SLOT(updateMargins()));
        });
    }

    connect(qApp, &QApplication::primaryScreenChanged, this, [=] { onGeometryChanged(); });
    connect(this, &QQuickWindow::activeChanged, this, &Launcher::onActiveChanged);

    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
        connect(kwin, &KWinWaylandInterface::clientAdded, this, [this](quint64 address) {
            focusWaylandWindow(address);
        });
    }
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

    if (!KWindowSystem::isPlatformX11()) {
        // setLaunchPad() above made the dock and the status bar give their
        // reserved area back. That has to reach KWin before the window is
        // mapped: the size KWin suggests in xdg_toplevel.configure_bounds is
        // the placement area, and Qt shrinks the window to it. Mapping first
        // meant the launcher came up at the work area size and was pushed back
        // out to the screen size a frame later, which is the flicker.
        KWinWaylandInterface::instance()->roundtrip();
    }

    if (!m_screenRect.isEmpty())
        setGeometry(m_screenRect);
    setVisible(true);
    setSurfacePosition(m_screenRect.topLeft());
    requestActivate();

    if (!KWindowSystem::isPlatformX11()) {
        // Qt's Wayland requestActivate() has no activation token when the
        // request originates from our D-Bus shortcut. Ask KWin's window
        // management protocol to focus this already-mapped surface after the
        // compositor has seen it.
        QTimer::singleShot(50, this, [this] {
            focusWaylandWindow();
        });
    }
}

void Launcher::focusWaylandWindow(quint64 address)
{
    if (KWindowSystem::isPlatformX11() || !m_showed)
        return;

    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    if (address == 0) {
        // The surface may not have been reported yet when showWindow() runs.
        // Refresh once here; future openings can use the cached client.
        kwin->refresh();
        const qint64 pid = QApplication::applicationPid();
        address = kwin->windowForPid(pid, QStringLiteral("Launcher"));
    }

    if (address != 0)
        kwin->focusWindow(address);
}

void Launcher::hideWindow()
{
    m_showed = false;
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
    return m_dockInterface.isValid();
}

bool Launcher::isPinedDock(const QString &desktop)
{
    QDBusInterface iface("com.cutefish.Dock",
                         "/Dock",
                         "com.cutefish.Dock",
                         QDBusConnection::sessionBus());

    if (!iface.isValid())
        return false;

    return iface.call("pinned", desktop).arguments().first().toBool();
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
    QRect dockGeometry = m_dockInterface.property("primaryGeometry").toRect();
    int dockDirection = m_dockInterface.property("direction").toInt();

    m_leftMargin = 0;
    m_rightMargin = 0;
    m_bottomMargin = 0;

    if (dockDirection == 0) {
        m_leftMargin = dockGeometry.width();
    } else if (dockDirection == 1) {
        m_bottomMargin = dockGeometry.height();
    } else if (dockDirection == 2) {
        m_rightMargin = dockGeometry.width();
    }

    emit marginsChanged();
}

void Launcher::updateSize()
{
    if (m_screenRect != qApp->primaryScreen()->geometry()) {
        m_screenRect = qApp->primaryScreen()->geometry();
        setGeometry(m_screenRect);
        setSurfacePosition(m_screenRect.topLeft());
        emit screenRectChanged();
    }
}

void Launcher::onGeometryChanged()
{
    disconnect(screen());

    setScreen(qApp->primaryScreen());
    updateSize();

    connect(screen(), &QScreen::virtualGeometryChanged, this, &Launcher::updateSize);
    connect(screen(), &QScreen::geometryChanged, this, &Launcher::updateSize);
}

void Launcher::showEvent(QShowEvent *e)
{
    if (KWindowSystem::isPlatformX11())
        KX11Extras::setState(winId(), NET::SkipTaskbar | NET::SkipPager);

    QQuickWindow::showEvent(e);
}

void Launcher::resizeEvent(QResizeEvent *e)
{
    // The launcher's QML root has no size of its own, so it has to follow the
    // window the way QQuickView's SizeRootObjectToView used to make it.
    ShellSurface::resizeEvent(e);

    // The window manager forces the size.
    e->ignore();
}

void Launcher::onActiveChanged()
{
    if (isActive())
        return;

    // A context menu is a separate Qt::Popup window. Opening it makes this
    // window inactive, but the launcher must remain visible while the menu is
    // open.
    QTimer::singleShot(0, this, [this] {
        if (isActive())
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
