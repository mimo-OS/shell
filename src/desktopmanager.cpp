#include "desktopmanager.h"
#include "desktopsurface.h"

#include <QGuiApplication>
#include <QScreen>

DesktopManager::DesktopManager(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
        screenAdded(screen);

    updatePrimaryScreen();

    connect(qApp, &QGuiApplication::screenAdded, this, &DesktopManager::screenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &DesktopManager::screenRemoved);
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &DesktopManager::updatePrimaryScreen);
}

DesktopManager::~DesktopManager()
{
    qDeleteAll(m_surfaces);
    m_surfaces.clear();
}

void DesktopManager::screenAdded(QScreen *screen)
{
    if (m_surfaces.contains(screen))
        return;

    DesktopSurface *surface = new DesktopSurface(m_engine, screen);
    m_surfaces.insert(screen, surface);

    // A new screen can become the primary one, and the surface has to know
    // before it is shown so it loads the right contents straight away.
    updatePrimaryScreen();
    surface->show();
}

void DesktopManager::screenRemoved(QScreen *screen)
{
    DesktopSurface *surface = m_surfaces.take(screen);

    if (!surface)
        return;

    surface->setVisible(false);
    surface->deleteLater();

    updatePrimaryScreen();
}

void DesktopManager::updatePrimaryScreen()
{
    QScreen *primary = QGuiApplication::primaryScreen();

    for (auto it = m_surfaces.constBegin(); it != m_surfaces.constEnd(); ++it)
        it.value()->setPrimary(it.key() == primary);
}
