#include "activity.h"
#include "kwinwaylandinterface.h"
#include "x11utils.h"

#include <QFile>
#include <QCursor>
#include <QDebug>
#include <QDirIterator>
#include <QSettings>
#include <QRegularExpression>
#include <QtGui/qguiapplication_platform.h>

#include <NETWM>
#include <KX11Extras>
#include <KWindowInfo>
#include <KWindowSystem>

// Every shell window (status bar, dock, launcher, desktop) now shares this
// WM_CLASS, so one entry blocks them all from being reported as "the active
// application".  xwaylandvideobridge is a transparent helper window used for
// screen sharing; it must not appear as the current application either.
static const QStringList blockList = {
    QStringLiteral("cutefish-shell"),
    QStringLiteral("xwaylandvideobridge"),
    QStringLiteral("xwayland-video-bridge"),
};

static bool isBlockedWaylandClient(const QString &appId)
{
    const QString normalized = appId.toLower();
    return blockList.contains(appId, Qt::CaseInsensitive)
        || normalized.contains(QStringLiteral("xwaylandvideobridge"))
        || normalized.contains(QStringLiteral("xwayland-video-bridge"));
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
    , m_cApps(CApplications::self())
    , m_existsWindowMaximized(false)
    , m_pid(0)
    , m_launchPad(false)
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
        connect(kwin, &KWinWaylandInterface::activeWindowChanged, this, &Activity::onActiveWindowChanged);
        connect(kwin, &KWinWaylandInterface::clientAdded, this, &Activity::updateExistsWindowMaximized);
        connect(kwin, &KWinWaylandInterface::clientRemoved, this, &Activity::updateExistsWindowMaximized);
        connect(kwin, &KWinWaylandInterface::clientChanged, this, &Activity::updateExistsWindowMaximized);
        connect(kwin, &KWinWaylandInterface::clientChanged, this, &Activity::onActiveWindowChanged);
        onActiveWindowChanged();
        return;
    }

    onActiveWindowChanged();

    connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this, &Activity::onActiveWindowChanged);
    connect(KX11Extras::self(), static_cast<void (KX11Extras::*)(WId id, NET::Properties properties, NET::Properties2 properties2)>(&KX11Extras::windowChanged),
            this, &Activity::onActiveWindowChanged);
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

// Whether anything is maximised decides if the dock may hide in "intelligent
// hide" mode, so it has to look at every window, not just the active one.
void Activity::updateExistsWindowMaximized()
{
    if (!KWindowSystem::isPlatformX11()) {
        bool existsWindowMaximized = false;
        for (const KWinWaylandClient &client : KWinWaylandInterface::instance()->clients()) {
            const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
            if (client.mapped && !isBlockedWaylandClient(appId)
                    && (client.maximized || client.fullscreen != 0)) {
                existsWindowMaximized = true;
                break;
            }
        }

        if (m_existsWindowMaximized != existsWindowMaximized) {
            m_existsWindowMaximized = existsWindowMaximized;
            emit existsWindowMaximizedChanged();
        }
        return;
    }

    bool existsWindowMaximized = false;

    for (WId wid : KX11Extras::windows()) {
        KWindowInfo i(wid, NET::WMState, NET::WM2WindowClass);

        if (i.isMinimized() || i.hasState(NET::SkipTaskbar))
            continue;

        if (i.hasState(NET::MaxVert) || i.hasState(NET::MaxHoriz)) {
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
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->closeWindow(KWinWaylandInterface::instance()->activeWindow());
        return;
    }

    const auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (native && native->connection())
        NETRootInfo(native->connection(), NET::CloseWindow).closeWindowRequest(KX11Extras::activeWindow());
}

void Activity::minimize()
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->minimizeWindow(KWinWaylandInterface::instance()->activeWindow());
        return;
    }

    KX11Extras::minimizeWindow(KX11Extras::activeWindow());
}

void Activity::restore()
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->restoreWindow(KWinWaylandInterface::instance()->activeWindow());
        return;
    }

    KX11Extras::clearState(KX11Extras::activeWindow(), NET::Max);
}

void Activity::maximize()
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->toggleMaximize(KWinWaylandInterface::instance()->activeWindow());
        return;
    }

    KX11Extras::setState(KX11Extras::activeWindow(), NET::Max);
}

void Activity::toggleMaximize()
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->toggleMaximize(KWinWaylandInterface::instance()->activeWindow());
        return;
    }

    KWindowInfo info(KX11Extras::activeWindow(), NET::WMState);
    bool isWindow = !info.hasState(NET::SkipTaskbar) ||
                     info.windowType(NET::UtilityMask) != NET::Utility ||
                     info.windowType(NET::DesktopMask) != NET::Desktop;

    if (!isWindow)
        return;

    bool isMaximized = info.hasState(NET::Max);
    isMaximized ? restore() : maximize();
}

void Activity::move()
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->startMove(KWinWaylandInterface::instance()->activeWindow());
        return;
    }

    WId winId = KX11Extras::activeWindow();
    KWindowInfo info(winId, NET::WMState | NET::WMGeometry | NET::WMDesktop);
    bool window = !info.hasState(NET::SkipTaskbar) ||
                     info.windowType(NET::UtilityMask) != NET::Utility ||
                     info.windowType(NET::DesktopMask) != NET::Desktop;

    if (!window)
        return;

    bool onCurrent = info.isOnCurrentDesktop();
    if (!onCurrent) {
        KX11Extras::setCurrentDesktop(info.desktop());
        KX11Extras::forceActiveWindow(winId);
    }

    const auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!native || !native->connection())
        return;

    NETRootInfo ri(native->connection(), NET::WMMoveResize);
    ri.moveResizeRequest(winId,
                         QCursor::pos().x(),
                         QCursor::pos().y(), NET::Move);
}

bool Activity::isAcceptableWindow(quint64 wid)
{
    if (!KWindowSystem::isPlatformX11()) {
        const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
        const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
        return client.id != 0 && client.mapped
            && !isBlockedWaylandClient(appId);
    }

    QFlags<NET::WindowTypeMask> ignoreList;
    ignoreList |= NET::DesktopMask;
    ignoreList |= NET::DockMask;
    ignoreList |= NET::SplashMask;
    ignoreList |= NET::ToolbarMask;
    ignoreList |= NET::MenuMask;
    ignoreList |= NET::PopupMenuMask;
    ignoreList |= NET::NotificationMask;

    KWindowInfo info(wid, NET::WMWindowType | NET::WMState, NET::WM2TransientFor | NET::WM2WindowClass);

    if (!info.valid())
        return false;

    if (NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList))
        return false;

    if (info.hasState(NET::SkipTaskbar) || info.hasState(NET::SkipPager))
        return false;

    // WM_TRANSIENT_FOR hint not set - normal window
    WId transFor = info.transientFor();
    if (transFor == 0 || transFor == wid || transFor == (WId) Cutefish::X11::rootWindow())
        return true;

    info = KWindowInfo(transFor, NET::WMWindowType);

    QFlags<NET::WindowTypeMask> normalFlag;
    normalFlag |= NET::NormalMask;
    normalFlag |= NET::DialogMask;
    normalFlag |= NET::UtilityMask;

    return !NET::typeMatchesMask(info.windowType(NET::AllTypesMask), normalFlag);
}

void Activity::onActiveWindowChanged()
{
    if (!KWindowSystem::isPlatformX11()) {
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

        CAppItem *item = m_cApps->matchItem(m_pid, m_windowClass);
        const QString title = item ? item->localName : client.title;
        const QString icon = item ? item->icon : m_windowClass;

        if (m_title != title) {
            m_title = title;
            emit titleChanged();
        }
        if (m_icon != icon) {
            m_icon = icon;
            emit iconChanged();
        }
        return;
    }

    KWindowInfo info(KX11Extras::activeWindow(),
                     NET::WMState | NET::WMVisibleName | NET::WMWindowType,
                     NET::WM2WindowClass);

    updateExistsWindowMaximized();

    if (NET::typeMatchesMask(info.windowType(NET::AllTypesMask), NET::DesktopMask)) {
        m_title = tr("Desktop");
        m_icon = "";

        emit titleChanged();
        emit iconChanged();

        return;
    }

    if (!isAcceptableWindow(KX11Extras::activeWindow())
            || blockList.contains(info.windowClassClass())) {
        clearTitle();
        clearIcon();
        return;
    }

    m_pid = info.pid();
    m_windowClass = info.windowClassClass().toLower();

    CAppItem *item = m_cApps->matchItem(m_pid, m_windowClass);

    if (item) {
        m_title = item->localName;
        emit titleChanged();

        if (m_icon != item->icon) {
            m_icon = item->icon;
            emit iconChanged();
        }

    } else {
        QString title = info.visibleName();
        if (title != m_title) {
            m_title = title;
            emit titleChanged();
            clearIcon();
        }
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
