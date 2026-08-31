#include "windowinterface.h"
#include "utils.h"

#include "kwinwaylandinterface.h"

static WindowInterface *INSTANCE = nullptr;

static bool isBlockedClient(const QString &appId)
{
    const QString normalized = appId.toLower();
    return normalized == QStringLiteral("cutefish-shell")
        || normalized == QStringLiteral("org.kde.xwaylandvideobridge");
}

WindowInterface *WindowInterface::instance()
{
    if (!INSTANCE)
        INSTANCE = new WindowInterface;
    return INSTANCE;
}

WindowInterface::WindowInterface(QObject *parent)
    : QObject(parent)
{
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    connect(kwin, &KWinWaylandInterface::clientAdded,
            this, &WindowInterface::onWindowAdded);
    connect(kwin, &KWinWaylandInterface::clientRemoved,
            this, &WindowInterface::windowRemoved);
    connect(kwin, &KWinWaylandInterface::clientChanged,
            this, &WindowInterface::onWindowChanged);
    connect(kwin, &KWinWaylandInterface::activeWindowChanged,
            this, &WindowInterface::activeChanged);

    for (const KWinWaylandClient &client : kwin->clients())
        onWindowAdded(client.id);
}

quint64 WindowInterface::activeWindow() const
{
    return KWinWaylandInterface::instance()->activeWindow();
}

void WindowInterface::minimizeWindow(quint64 window)
{
    KWinWaylandInterface::instance()->minimizeWindow(window);
}

void WindowInterface::closeWindow(quint64 window)
{
    KWinWaylandInterface::instance()->closeWindow(window);
}

void WindowInterface::forceActiveWindow(quint64 window)
{
    KWinWaylandInterface::instance()->focusWindow(window);
}

QMap<QString, QVariant> WindowInterface::requestInfo(quint64 wid) const
{
    const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
    if (client.id == 0)
        return {};

    const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
    QMap<QString, QVariant> result;
    result.insert("iconName", appId.toLower());
    result.insert("active", wid == activeWindow());
    result.insert("visibleName", client.title);
    result.insert("id", appId);
    result.insert("pid", client.pid);
    return result;
}

QString WindowInterface::requestWindowClass(quint64 wid) const
{
    const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
    return client.className.isEmpty() ? client.initialClass : client.className;
}

bool WindowInterface::isAcceptableWindow(quint64 wid) const
{
    const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
    const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
    return client.id != 0 && client.mapped && !isBlockedClient(appId);
}

void WindowInterface::startInitWindows()
{
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    kwin->refresh();
    for (const KWinWaylandClient &client : kwin->clients())
        onWindowAdded(client.id);
}

QString WindowInterface::desktopFilePath(quint64 wid) const
{
    const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
    const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
    return Utils::instance()->desktopPathFromMetadata(appId, client.pid, appId);
}

void WindowInterface::onWindowAdded(quint64 wid)
{
    if (isAcceptableWindow(wid))
        emit windowAdded(wid);
}

void WindowInterface::onWindowChanged(quint64 wid)
{
    if (isAcceptableWindow(wid))
        emit windowChanged(wid);
    else
        emit windowRemoved(wid);
}
