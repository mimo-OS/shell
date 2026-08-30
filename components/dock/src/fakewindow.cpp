#include "fakewindow.h"
#include "docksettings.h"

// Qt
#include <QDebug>
#include <QApplication>
#include <QSurfaceFormat>
#include <QQuickView>
#include <QScreen>
#include <QTimer>

#include <KX11Extras>
#include <KWindowSystem>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#include "kwinwaylandinterface.h"

// X11
#include <NETWM>

FakeWindow::FakeWindow(QQuickView *parent)
    : QQuickView(parent)
    , m_delayedContainsMouse(false)
    , m_containsMouse(false)
{
    setColor(Qt::transparent);
    setDefaultAlphaBuffer(true);
    setFlags(Qt::FramelessWindowHint |
             Qt::WindowStaysOnTopHint |
             Qt::NoDropShadowWindowHint |
             Qt::WindowDoesNotAcceptFocus);
    // setScreen(qApp->primaryScreen());
    // updateGeometry();
    show();

    m_delayedMouseTimer.setSingleShot(true);
    m_delayedMouseTimer.setInterval(50);
    connect(&m_delayedMouseTimer, &QTimer::timeout, this, [this]() {
        if (m_delayedContainsMouse) {
            setContainsMouse(true);
        } else {
            setContainsMouse(false);
        }
    });

    connect(DockSettings::self(), &DockSettings::directionChanged, this, &FakeWindow::updateGeometry);
}

bool FakeWindow::containsMouse() const
{
    return m_containsMouse;
}

bool FakeWindow::event(QEvent *e)
{
    if (e->type() == QEvent::DragEnter || e->type() == QEvent::DragMove) {
        if (!m_containsMouse) {
            m_delayedContainsMouse = false;
            m_delayedMouseTimer.stop();
            setContainsMouse(true);
            emit dragEntered();
        }
    } else if (e->type() == QEvent::Enter) {
        m_delayedContainsMouse = true;
        if (!m_delayedMouseTimer.isActive()) {
            m_delayedMouseTimer.start();
        }
    } else if (e->type() == QEvent::Leave || e->type() == QEvent::DragLeave) {
        m_delayedContainsMouse = false;
        if (!m_delayedMouseTimer.isActive()) {
            m_delayedMouseTimer.start();
        }
    } else if (e->type() == QEvent::Show) {
        if (KWindowSystem::isPlatformX11())
            KX11Extras::setState(winId(), NET::SkipTaskbar | NET::SkipPager | NET::SkipSwitcher);
    } else if (e->type() == QEvent::Expose) {
        // The wl_surface only exists once the window is exposed, and Qt makes
        // a new one every time it is hidden and shown again.
        if (isExposed())
            updatePlasmaSurface();
    }

    return QQuickView::event(e);
}

void FakeWindow::setContainsMouse(bool contains)
{
    if (m_containsMouse != contains) {
        m_containsMouse = contains;
        emit containsMouseChanged(contains);
    }
}

void FakeWindow::updatePlasmaSurface()
{
    if (KWindowSystem::isPlatformX11())
        return;

    KWayland::Client::PlasmaShell *shell = KWinWaylandInterface::instance()->plasmaShell();
    if (!shell)
        return;

    KWayland::Client::Surface *surface = KWayland::Client::Surface::fromWindow(this);
    if (!surface)
        return;

    delete m_plasmaSurface;
    m_plasmaSurface = shell->createSurface(surface, this);
    if (!m_plasmaSurface)
        return;

    // The auto-hide trigger is a thin strip on the screen edge. It has to sit
    // above ordinary windows to catch the pointer, but must never reserve any
    // work area of its own.
    m_plasmaSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Panel);
    m_plasmaSurface->setPanelBehavior(KWayland::Client::PlasmaShellSurface::PanelBehavior::WindowsGoBelow);
    m_plasmaSurface->setPanelTakesFocus(false);
    m_plasmaSurface->setSkipTaskbar(true);
    m_plasmaSurface->setSkipSwitcher(true);
    m_plasmaSurface->setPosition(geometry().topLeft());
}

void FakeWindow::updateGeometry()
{

    int length = 5;
    const QRect screenRect = qApp->primaryScreen()->geometry();
    QRect newRect;

    switch (DockSettings::self()->direction())
    {
        case DockSettings::Left:
            newRect = QRect(screenRect.x() - (length * 2),
                            (screenRect.height() + length) / 2,
                            length, screenRect.height());
            break;
        case DockSettings::Bottom:
            newRect = QRect(screenRect.x(),
                            screenRect.y() + screenRect.height() - length,
                            screenRect.width(), length);
            break;
        case DockSettings::Right:
            newRect = QRect(screenRect.x() + screenRect.width() - length,
                            screenRect.y(),
                            length, screenRect.height());
            break;
    }

    setGeometry(newRect);
    updatePlasmaSurface();
}
