#include "activity.h"
#include "applicationresolver.h"
#include "applicationregistry.h"
#include "kwinwaylandinterface.h"

static bool isBlockedClient(const QString &appId)
{
    const QString normalized = appId.toLower();
    return normalized == QStringLiteral("cutefish-shell")
        || normalized == QStringLiteral("org.kde.xwaylandvideobridge");
}

static Activity *SELF = nullptr;

Activity *Activity::self()
{
    if (!SELF)
        SELF = new Activity;
    return SELF;
}

Activity::Activity(QObject *parent)
    : QObject(parent)
    , m_existsWindowMaximized(false)
    , m_pid(0)
    , m_launchPad(false)
{
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    ApplicationRegistry *registry = ApplicationRegistry::instance();
    connect(registry, &ApplicationRegistry::applicationsChanged,
            this, &Activity::onActiveWindowChanged);
    connect(kwin, &KWinWaylandInterface::activeWindowChanged,
            this, &Activity::onActiveWindowChanged);
    connect(kwin, &KWinWaylandInterface::clientAdded,
            this, &Activity::updateExistsWindowMaximized);
    connect(kwin, &KWinWaylandInterface::clientRemoved,
            this, &Activity::updateExistsWindowMaximized);
    connect(kwin, &KWinWaylandInterface::clientChanged,
            this, &Activity::updateExistsWindowMaximized);
    connect(kwin, &KWinWaylandInterface::clientChanged,
            this, &Activity::onActiveWindowChanged);
    onActiveWindowChanged();
}

bool Activity::launchPad() const
{
    return m_launchPad;
}

void Activity::setLaunchPad(bool launchPad)
{
    if (m_launchPad == launchPad)
        return;
    m_launchPad = launchPad;
    emit launchPadChanged();
}

bool Activity::existsWindowMaximized() const
{
    return m_existsWindowMaximized;
}

void Activity::updateExistsWindowMaximized()
{
    bool existsWindowMaximized = false;
    for (const KWinWaylandClient &client : KWinWaylandInterface::instance()->clients()) {
        const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
        if (client.mapped && !isBlockedClient(appId)
                && (client.maximized || client.fullscreen != 0)) {
            existsWindowMaximized = true;
            break;
        }
    }

    if (m_existsWindowMaximized != existsWindowMaximized) {
        m_existsWindowMaximized = existsWindowMaximized;
        emit existsWindowMaximizedChanged();
    }
}

QString Activity::title() const
{
    return m_title;
}

QString Activity::icon() const
{
    return m_icon;
}

void Activity::close()
{
    KWinWaylandInterface::instance()->closeWindow(
        KWinWaylandInterface::instance()->activeWindow());
}

void Activity::minimize()
{
    KWinWaylandInterface::instance()->minimizeWindow(
        KWinWaylandInterface::instance()->activeWindow());
}

void Activity::restore()
{
    KWinWaylandInterface::instance()->restoreWindow(
        KWinWaylandInterface::instance()->activeWindow());
}

void Activity::maximize()
{
    KWinWaylandInterface::instance()->toggleMaximize(
        KWinWaylandInterface::instance()->activeWindow());
}

void Activity::toggleMaximize()
{
    maximize();
}

void Activity::move()
{
    KWinWaylandInterface::instance()->startMove(
        KWinWaylandInterface::instance()->activeWindow());
}

bool Activity::isAcceptableWindow(quint64 wid)
{
    const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
    const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
    return client.id != 0 && client.mapped && !isBlockedClient(appId);
}

void Activity::onActiveWindowChanged()
{
    updateExistsWindowMaximized();

    const quint64 address = KWinWaylandInterface::instance()->activeWindow();
    const KWinWaylandClient client = KWinWaylandInterface::instance()->client(address);
    if (client.id == 0 || !isAcceptableWindow(address)) {
        clearTitle();
        clearIcon();
        return;
    }

    m_pid = static_cast<quint32>(client.pid);
    m_windowClass = (client.className.isEmpty() ? client.initialClass : client.className).toLower();

    DesktopEntry *entry = ApplicationResolver::instance()->resolve(
        client.className.isEmpty() ? client.initialClass : client.className,
        m_pid,
        m_windowClass);
    const QString title = entry ? entry->name() : client.title;
    const QString icon = entry ? entry->icon() : m_windowClass;

    if (m_title != title) {
        m_title = title;
        emit titleChanged();
    }
    if (m_icon != icon) {
        m_icon = icon;
        emit iconChanged();
    }
}

void Activity::clearTitle()
{
    m_title.clear();
    emit titleChanged();
}

void Activity::clearIcon()
{
    m_icon.clear();
    emit iconChanged();
}
