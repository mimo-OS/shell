#include "xwindowinterface.h"
#include "utils.h"
#include "x11utils.h"

#include <QTimer>
#include <QDebug>
#include <QWindow>
#include <QScreen>
#include <QtGui/qguiapplication_platform.h>

#include <KWindowEffects>
#include <KX11Extras>
#include <KWindowInfo>
#include <KWindowSystem>

// X11
#include <NETWM>

static XWindowInterface *INSTANCE = nullptr;

static bool isBlockedWaylandClient(const QString &appId)
{
    const QString normalized = appId.toLower();
    return normalized == QStringLiteral("cutefish-shell")
        || normalized.contains(QStringLiteral("xwaylandvideobridge"))
        || normalized.contains(QStringLiteral("xwayland-video-bridge"));
}

XWindowInterface *XWindowInterface::instance()
{
    if (!INSTANCE)
        INSTANCE = new XWindowInterface;

    return INSTANCE;
}

XWindowInterface::XWindowInterface(QObject *parent)
    : QObject(parent)
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
        if (!kwin->available())
            return;

        connect(kwin, &KWinWaylandInterface::clientAdded, this, &XWindowInterface::onWaylandWindowAdded);
        connect(kwin, &KWinWaylandInterface::clientRemoved, this, &XWindowInterface::windowRemoved);
        connect(kwin, &KWinWaylandInterface::clientChanged, this, &XWindowInterface::onWaylandWindowChanged);
        connect(kwin, &KWinWaylandInterface::activeWindowChanged, this, &XWindowInterface::activeChanged);
        connect(kwin, &KWinWaylandInterface::windowManagementReady, this, [this, kwin] {
            for (const KWinWaylandClient &client : kwin->clients())
                onWaylandWindowAdded(client.id);
        });
        for (const KWinWaylandClient &client : kwin->clients())
            onWaylandWindowAdded(client.id);
        return;
    }

    connect(KX11Extras::self(), &KX11Extras::windowAdded, this, &XWindowInterface::onWindowadded);
    connect(KX11Extras::self(), &KX11Extras::windowRemoved, this, &XWindowInterface::windowRemoved);
    connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this, &XWindowInterface::activeChanged);
}

void XWindowInterface::enableBlurBehind(QWindow *view, bool enable, const QRegion &region)
{
    if (!KWindowSystem::isPlatformX11())
        return;

    KWindowEffects::enableBlurBehind(view, enable, region);
}

WId XWindowInterface::activeWindow()
{
    if (!KWindowSystem::isPlatformX11())
        return KWinWaylandInterface::instance()->activeWindow();

    return KX11Extras::activeWindow();
}

void XWindowInterface::minimizeWindow(WId win)
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->minimizeWindow(win);
        return;
    }

    KX11Extras::minimizeWindow(win);
}

void XWindowInterface::closeWindow(WId id)
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->closeWindow(id);
        return;
    }

    // FIXME: Why there is no such thing in KWindowSystem??
    const auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (native && native->connection())
        NETRootInfo(native->connection(), NET::CloseWindow).closeWindowRequest(id);
}

void XWindowInterface::forceActiveWindow(WId win)
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface::instance()->focusWindow(win);
        return;
    }

    KX11Extras::forceActiveWindow(win);
}

QMap<QString, QVariant> XWindowInterface::requestInfo(quint64 wid)
{
    if (!KWindowSystem::isPlatformX11()) {
        const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
        if (client.id == 0)
            return {};

        const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
        QMap<QString, QVariant> result;
        result.insert("iconName", appId.toLower());
        result.insert("active", wid == KWinWaylandInterface::instance()->activeWindow());
        result.insert("visibleName", client.title);
        result.insert("id", appId);
        result.insert("pid", client.pid);
        return result;
    }

    const KWindowInfo winfo { wid, NET::WMFrameExtents
                | NET::WMWindowType
                | NET::WMGeometry
                | NET::WMDesktop
                | NET::WMState
                | NET::WMName
                | NET::WMVisibleName,
                NET::WM2WindowClass
                | NET::WM2Activities
                | NET::WM2AllowedActions
                | NET::WM2TransientFor };
    QMap<QString, QVariant> result;
    const QString winClass = QString(winfo.windowClassClass());

    result.insert("iconName", winClass.toLower());
    result.insert("active", wid == KX11Extras::activeWindow());
    result.insert("visibleName", winfo.visibleName());
    result.insert("id", winClass);

    return result;
}

QString XWindowInterface::requestWindowClass(quint64 wid)
{
    if (!KWindowSystem::isPlatformX11()) {
        const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
        return client.className.isEmpty() ? client.initialClass : client.className;
    }

    return KWindowInfo(wid, NET::Supported, NET::WM2WindowClass).windowClassClass();
}

bool XWindowInterface::isAcceptableWindow(quint64 wid)
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

void XWindowInterface::setViewStruts(QWindow *view, DockSettings::Direction direction, const QRect &rect, bool compositing)
{
    if (!KWindowSystem::isPlatformX11())
        return;

    NETExtendedStrut strut;

    const auto screen = view->screen();

    // const QRect currentScreen {screen->geometry()};
    const QRect wholeScreen { {0, 0}, screen->virtualSize() };
    const int edgeMargins = compositing ? DockSettings::self()->edgeMargins() : 0;

    switch (direction) {
    case DockSettings::Left: {
        const int leftOffset = { screen->geometry().left() };
        strut.left_width = rect.width() + leftOffset + edgeMargins;
        strut.left_start = rect.y();
        strut.left_end = rect.y() + rect.height() - 1;
        break;
    }
    case DockSettings::Bottom: {
        strut.bottom_width = rect.height() + edgeMargins;
        strut.bottom_start = rect.x();
        strut.bottom_end = rect.x() + rect.width();
        break;
    }
    case DockSettings::Right: {
        // const int rightOffset = {wholeScreen.right() - currentScreen.right()};
        strut.right_width = rect.width() + edgeMargins;
        strut.right_start = rect.y();
        strut.right_end = rect.y() + rect.height() - 1;
        break;
    }
    default:
        break;
    }

    KX11Extras::setExtendedStrut(view->winId(),
                                    strut.left_width,   strut.left_start,   strut.left_end,
                                    strut.right_width,  strut.right_start,  strut.right_end,
                                    strut.top_width,    strut.top_start,    strut.top_end,
                                    strut.bottom_width, strut.bottom_start, strut.bottom_end
                                    );
}

void XWindowInterface::clearViewStruts(QWindow *view)
{
    if (!KWindowSystem::isPlatformX11())
        return;

    KX11Extras::setExtendedStrut(view->winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void XWindowInterface::startInitWindows()
{
    if (!KWindowSystem::isPlatformX11()) {
        KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
        kwin->refresh();
        for (const KWinWaylandClient &client : kwin->clients())
            onWaylandWindowAdded(client.id);
        return;
    }

    for (auto wid : KX11Extras::self()->windows()) {
        onWindowadded(wid);
    }
}

QString XWindowInterface::desktopFilePath(quint64 wid)
{
    if (!KWindowSystem::isPlatformX11()) {
        const KWinWaylandClient client = KWinWaylandInterface::instance()->client(wid);
        const QString appId = client.className.isEmpty() ? client.initialClass : client.className;
        return Utils::instance()->desktopPathFromMetadata(appId, client.pid, appId);
    }

    const auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!native || !native->connection())
        return {};

    const KWindowInfo info(wid, NET::Properties(), NET::WM2WindowClass | NET::WM2DesktopFileName);
    return Utils::instance()->desktopPathFromMetadata(info.windowClassClass(),
                                                      NETWinInfo(native->connection(), wid,
                                                                 Cutefish::X11::rootWindow(),
                                                                 NET::WMPid,
                                                                 NET::Properties2()).pid(),
                                                      info.windowClassName());
}

void XWindowInterface::setIconGeometry(quint64 wid, const QRect &rect)
{
    if (!KWindowSystem::isPlatformX11())
        return;

    const auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!native || !native->connection())
        return;

    NETWinInfo info(native->connection(),
                    wid,
                    (WId) Cutefish::X11::rootWindow(),
                    NET::WMIconGeometry,
                    QFlags<NET::Property2>(1));
    NETRect nrect;
    nrect.pos.x = rect.x();
    nrect.pos.y = rect.y();
    nrect.size.height = rect.height();
    nrect.size.width = rect.width();
    info.setIconGeometry(nrect);
}

void XWindowInterface::onWindowadded(quint64 wid)
{
    if (!KWindowSystem::isPlatformX11())
        return;

    if (isAcceptableWindow(wid)) {
        emit windowAdded(wid);
    }
}

void XWindowInterface::onWaylandWindowAdded(quint64 wid)
{
    if (isAcceptableWindow(wid))
        emit windowAdded(wid);
}

void XWindowInterface::onWaylandWindowChanged(quint64 wid)
{
    if (isAcceptableWindow(wid))
        emit windowChanged(wid);
    else
        emit windowRemoved(wid);
}
