#include "activity.h"
#include "applicationresolver.h"
#include "applicationregistry.h"
#include "applicationruntime.h"
#include "core/wayland/kwinwaylandinterface.h"

#include <QCoreApplication>
#include <QTimer>

// Let the application close its windows on its own before the runtime
// terminates whatever is left of it.
static const int kQuitGraceMs = 3000;

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
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    bool existsWindowMaximized = false;
    for (const KWinWaylandClient &client : kwin->clients()) {
        if (!client.hidden && client.mapped && !client.skipTaskbar
                && (client.maximized || client.fullscreen != 0)
                && kwin->isAcceptableWindow(client.id)) {
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

void Activity::quit()
{
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    const quint64 window = kwin->activeWindow();

    if (!kwin->isAcceptableWindow(window))
        return;

    const quint32 pid = m_pid;

    // Never terminate the shell itself.
    if (pid == static_cast<quint32>(QCoreApplication::applicationPid()))
        return;

    // Quit means the application, not this one window: close every window it
    // has so it can still prompt to save.
    if (pid != 0) {
        for (const KWinWaylandClient &client : kwin->clients()) {
            if (static_cast<quint32>(client.pid) == pid && kwin->isAcceptableWindow(client.id))
                kwin->closeWindow(client.id);
        }
    } else {
        kwin->closeWindow(window);
        return;
    }

    // Whatever is left after that is terminated, including an application that
    // closed its windows but keeps running.
    QTimer::singleShot(kQuitGraceMs, this, [pid] {
        ApplicationRuntime::instance()->quitByPid(pid);
    });
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

void Activity::hideOthers()
{
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    const quint64 active = kwin->activeWindow();
    for (const KWinWaylandClient &client : kwin->clients()) {
        if (client.id != active && kwin->isAcceptableWindow(client.id) && !client.hidden) {
            kwin->minimizeWindow(client.id);
        }
    }
}

void Activity::showAll()
{
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    for (const KWinWaylandClient &client : kwin->clients()) {
        if (kwin->isAcceptableWindow(client.id) && client.hidden) {
            kwin->restoreWindow(client.id);
        }
    }
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

void Activity::onActiveWindowChanged()
{
    updateExistsWindowMaximized();

    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    const quint64 address = kwin->activeWindow();
    const KWinWaylandClient client = kwin->client(address);
    if (!kwin->isAcceptableWindow(address)) {
        clearTitle();
        clearIcon();
        return;
    }

    const QString appId = KWinWaylandInterface::appId(client);
    m_pid = static_cast<quint32>(client.pid);
    m_windowClass = appId.toLower();

    DesktopEntry *entry = ApplicationResolver::instance()->resolve(appId, m_pid, m_windowClass);
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
