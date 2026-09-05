#include "desktopsurface.h"
#include "core/wayland/kwinwaylandinterface.h"

#include <QCoreApplication>
#include <QScreen>
#include <QQmlContext>
#include <QTimer>

DesktopSurface::DesktopSurface(QQmlEngine *engine, QScreen *screen)
    : ShellSurface(engine)
    , m_screen(screen)
    , m_primary(false)
    , m_keyboardClaimed(false)
{
    setFlag(Qt::FramelessWindowHint);
    setColor(Qt::transparent);

    setTitle(QStringLiteral("Desktop"));
    QQuickWindow::setScreen(screen);
    configureSurfaceRole(SurfaceRole::Desktop);
    updateGeometry();

    setContextProperty(QStringLiteral("surface"), this);
    setSource(QUrl(QStringLiteral("qrc:/shell/qml/DesktopSurface.qml")));

    watchScreen(m_screen, [this] { updateGeometry(); });
}

QScreen *DesktopSurface::targetScreen() const
{
    return m_screen.data();
}

bool DesktopSurface::isPrimary() const
{
    return m_primary;
}

void DesktopSurface::setPrimary(bool primary)
{
    if (m_primary == primary)
        return;

    m_primary = primary;
    emit primaryChanged();

    if (m_primary && isExposed())
        claimKeyboard();
}

void DesktopSurface::exposeEvent(QExposeEvent *event)
{
    ShellSurface::exposeEvent(event);

    if (m_primary && isExposed())
        claimKeyboard();
}

// Right after login KWin has no active window at all, so the desktop gets no
// key events until it is clicked once: arrow keys, Enter and F2 all do nothing.
// Take the keyboard once, when the desktop first appears.
void DesktopSurface::claimKeyboard()
{
    if (m_keyboardClaimed)
        return;

    m_keyboardClaimed = true;

    requestActivate();

    // requestActivate() has no activation token to offer when it does not come
    // from an input event, so ask KWin directly once it knows the surface.
    QTimer::singleShot(50, this, [this] {
        KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
        kwin->refresh();

        const quint64 id = kwin->windowForPid(QCoreApplication::applicationPid(),
                                              QStringLiteral("Desktop"));

        if (id != 0)
            kwin->focusWindow(id);
    });
}

void DesktopSurface::updateGeometry()
{
    if (!m_screen)
        return;

    const QRect rect = m_screen->geometry();

    setGeometry(rect);
    setSurfacePosition(rect.topLeft());
}
