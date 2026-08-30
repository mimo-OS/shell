#include "desktopsurface.h"

#include <QScreen>
#include <QQmlContext>

#include <NETWM>
#include <KX11Extras>

DesktopSurface::DesktopSurface(QQmlEngine *engine, QScreen *screen)
    : ShellSurface(engine)
    , m_screen(screen)
    , m_primary(false)
{
    setFlag(Qt::FramelessWindowHint);
    setColor(Qt::transparent);

    KX11Extras::setType(winId(), NET::Desktop);
    KX11Extras::setState(winId(), NET::KeepBelow);

    // An opacity just below 1 keeps the compositor from treating the desktop as
    // fully opaque, which used to leave the wallpaper black on some drivers.
    setOpacity(0.99);

    setTitle(tr("Desktop"));
    QQuickWindow::setScreen(screen);
    updateGeometry();

    setContextProperty(QStringLiteral("surface"), this);
    setSource(QUrl(QStringLiteral("qrc:/shell/qml/DesktopSurface.qml")));

    connect(m_screen, &QScreen::geometryChanged, this, &DesktopSurface::updateGeometry);
    connect(m_screen, &QScreen::virtualGeometryChanged, this, &DesktopSurface::updateGeometry);
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
}

void DesktopSurface::updateGeometry()
{
    if (!m_screen)
        return;

    // The extra pixel matches what the standalone desktop did: without it a
    // one pixel line of the root window shows through at the right/bottom edge.
    setGeometry(m_screen->geometry().adjusted(0, 0, 1, 1));
}
