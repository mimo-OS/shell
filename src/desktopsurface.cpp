#include "desktopsurface.h"

#include <QScreen>
#include <QQmlContext>

DesktopSurface::DesktopSurface(QQmlEngine *engine, QScreen *screen)
    : ShellSurface(engine)
    , m_screen(screen)
    , m_primary(false)
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
}

void DesktopSurface::updateGeometry()
{
    if (!m_screen)
        return;

    const QRect rect = m_screen->geometry();

    setGeometry(rect);
    setSurfacePosition(rect.topLeft());
}
