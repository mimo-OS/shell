#include "dockwindow.h"
#include "dockadaptor.h"

#include <QGuiApplication>
#include <QScreen>

#include <QQmlEngine>
#include <QQuickItem>
#include <QExposeEvent>

#include <KWayland/Client/surface.h>

#include "core/wayland/kwinwaylandinterface.h"

DockWindow::DockWindow(QQmlEngine *engine)
    : ShellSurface(engine)
    , m_activity(Activity::self())
    , m_settings(DockSettings::self())
    , m_appModel(new ApplicationModel(this))
    , m_autoHideTrigger(nullptr)
    , m_strutSpacer(new StrutSpacer(this))
    , m_trashManager(new TrashManager(this))
    , m_hideBlocked(false)
    , m_mouseOverDock(false)
    , m_mouseOverTrigger(false)
    , m_showTimer(new QTimer(this))
    , m_hideTimer(new QTimer(this))
    , m_geometryAnimation(new QVariantAnimation(this))
    , m_slideAnimation(new QVariantAnimation(this))
{
    new DockAdaptor(this);

    installEventFilter(this);

    setColor(Qt::transparent);

    setFlags(Qt::FramelessWindowHint);
    setTitle(QStringLiteral("Dock"));

    setContextProperty("appModel", m_appModel);
    setContextProperty("Settings", m_settings);
    setContextProperty("dock", this);
    setContextProperty("trash", m_trashManager);

    setScreen(qGuiApp->primaryScreen());
    updateMetrics();
    setSource(QUrl(QStringLiteral("qrc:/dock/qml/main.qml")));
    configureSurfaceRole(SurfaceRole::Dock);

    // Puts the dock on the primary screen and keeps it there through screen
    // and resolution changes.
    followPrimaryScreen([this] {
        if (m_autoHideTrigger) {
            m_autoHideTrigger->setScreen(screen());
            m_autoHideTrigger->updateGeometry();
        }
        resizeWindow();
    });

    onVisibilityChanged();

    m_showTimer->setSingleShot(true);
    m_showTimer->setInterval(10);
    connect(m_showTimer, &QTimer::timeout, this, &DockWindow::slideToShow);

    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(420);
    connect(m_hideTimer, &QTimer::timeout, this, &DockWindow::onHideTimeout);

    connect(m_slideAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
                setDockGeometry(value.toRect(), true);
            });
    connect(m_slideAnimation, &QVariantAnimation::finished, this, [this] {
        if (m_slideState == SlidingOut) {
            setVisible(false);
            m_slideState = Idle;
        } else if (m_slideState == SlidingIn) {
            m_slideState = Idle;
        }
    });

    m_geometryAnimation->setDuration(180);
    m_geometryAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_geometryAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
                setDockGeometry(value.toRect(), false);

                const qreal linearProgress = qBound<qreal>(
                    0.0,
                    static_cast<qreal>(m_geometryAnimation->currentTime())
                        / m_geometryAnimation->duration(),
                    1.0);
                setGeometryAnimationProgress(
                    m_geometryAnimation->easingCurve().valueForProgress(linearProgress));
            });
    connect(m_geometryAnimation, &QVariantAnimation::finished, this,
            [this] { setGeometryAnimationProgress(1.0); });

    // Track window geometry and touching changes via KWin Wayland
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    connect(kwin, &KWinWaylandInterface::windowManagementReady, this, &DockWindow::checkWindowOverlap);
    connect(kwin, &KWinWaylandInterface::clientAdded, this, &DockWindow::checkWindowOverlap);
    connect(kwin, &KWinWaylandInterface::clientRemoved, this, &DockWindow::checkWindowOverlap);
    connect(kwin, &KWinWaylandInterface::clientChanged, this, &DockWindow::checkWindowOverlap);
    connect(kwin, &KWinWaylandInterface::activeWindowChanged, this, &DockWindow::checkWindowOverlap);

    // When the current window changes.
    connect(m_activity, &Activity::launchPadChanged, this, &DockWindow::onVisibilityChanged);
    connect(m_activity, &Activity::existsWindowMaximizedChanged, this, &DockWindow::checkWindowOverlap);

    connect(m_appModel, &ApplicationModel::countChanged, this,
            [this] { resizeWindow(m_appModel->initialWindowsLoaded()); });
    connect(m_appModel, &ApplicationModel::itemAdded, this, [this] {
        if (m_appModel->initialWindowsLoaded()) {
            m_geometryAnimationAdding = true;
            ++m_geometryAnimationToken;
            emit geometryAnimationAddingChanged();
            emit geometryAnimationTokenChanged();
            setGeometryAnimationProgress(0.0);
        }
    });
    connect(m_appModel, &ApplicationModel::itemRemoved, this, [this] {
        if (m_appModel->initialWindowsLoaded() && m_geometryAnimationAdding) {
            m_geometryAnimationAdding = false;
            emit geometryAnimationAddingChanged();
        }
    });
    connect(m_settings, &DockSettings::directionChanged, this, &DockWindow::onPositionChanged);
    connect(m_settings, &DockSettings::iconSizeChanged, this, &DockWindow::onIconSizeChanged);
    connect(m_settings, &DockSettings::visibilityChanged, this, &DockWindow::onVisibilityChanged);

    checkWindowOverlap();
}

DockWindow::~DockWindow()
{
}

void DockWindow::add(const QString &desktop)
{
    m_appModel->addItem(desktop);
}

void DockWindow::remove(const QString &desktop)
{
    m_appModel->removeItem(desktop);
}

bool DockWindow::pinned(const QString &desktop)
{
    return m_appModel->isDesktopPinned(desktop);
}

QRect DockWindow::primaryGeometry() const
{
    return geometry();
}

int DockWindow::direction() const
{
    return DockSettings::self()->direction();
}

int DockWindow::visibility() const
{
    return DockSettings::self()->visibility();
}

qreal DockWindow::geometryAnimationProgress() const
{
    return m_geometryAnimationProgress;
}

bool DockWindow::geometryAnimationAdding() const
{
    return m_geometryAnimationAdding;
}

int DockWindow::geometryAnimationToken() const
{
    return m_geometryAnimationToken;
}

void DockWindow::setDirection(int direction)
{
    DockSettings::self()->setDirection(static_cast<DockSettings::Direction>(direction));
}

void DockWindow::setIconSize(int iconSize)
{
    DockSettings::self()->setIconSize(iconSize);
}

void DockWindow::setVisibility(int visibility)
{
    DockSettings::self()->setVisibility(static_cast<DockSettings::Visibility>(visibility));
}

void DockWindow::setLauncher(Launcher *launcher)
{
    m_appModel->setLauncher(launcher);
}

void DockWindow::updateIconGeometry(QQuickItem *item, const QStringList &windowIds)
{
    if (!item || item->width() <= 0 || item->height() <= 0)
        return;

    QQuickWindow *itemWindow = item->window();
    if (!itemWindow)
        return;

    KWayland::Client::Surface *surface = KWayland::Client::Surface::fromWindow(itemWindow);
    if (!surface)
        return;

    const QRect geometry = item->mapRectToScene(
        QRectF(0, 0, item->width(), item->height())).toRect();
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();

    for (const QString &windowId : windowIds) {
        bool ok = false;
        const quint64 id = windowId.toULongLong(&ok);
        if (ok)
            kwin->setMinimizedGeometry(id, surface, geometry);
    }
}

QVariantMap DockWindow::metrics() const
{
    return m_metrics.toVariantMap();
}

QRect DockWindow::windowRect() const
{
    QScreen *s = screen();
    if (!s)
        s = qGuiApp->primaryScreen();
    if (!s)
        return QRect();

    const QRect screenGeometry = s->geometry();
    const QRect availableGeometry = s->availableGeometry();

    // Add the trash item.
    const int itemCount = m_appModel->rowCount() + 1;
    const int margins = m_settings->edgeMargins() / 2;
    const int maxLength = (m_settings->direction() == DockSettings::Bottom)
        ? (screenGeometry.width() - margins * 2)
        : (availableGeometry.height() - margins * 2);
    const int length = qMin(m_metrics.length(itemCount), maxLength);
    const int thickness = m_metrics.thickness();

    QSize newSize(0, 0);
    QPoint position(0, 0);

    switch (m_settings->direction()) {
    case DockSettings::Left:
        newSize = QSize(thickness, length);
        position.setX(screenGeometry.x() + margins);
        position.setY(qMax(availableGeometry.y() + margins,
                           availableGeometry.y() + (availableGeometry.height() - newSize.height()) / 2));
        break;
    case DockSettings::Bottom:
        newSize = QSize(length, thickness);
        position.setX(qMax(screenGeometry.x() + margins,
                           screenGeometry.x() + (screenGeometry.width() - newSize.width()) / 2));
        position.setY(screenGeometry.y() + screenGeometry.height() - newSize.height() - margins);
        break;
    case DockSettings::Right:
        newSize = QSize(thickness, length);
        position.setX(screenGeometry.x() + screenGeometry.width() - newSize.width() - margins);
        position.setY(qMax(availableGeometry.y() + margins,
                           availableGeometry.y() + (availableGeometry.height() - newSize.height()) / 2));
        break;
    default:
        break;
    }

    return QRect(position, newSize);
}

QRect DockWindow::hiddenWindowRect() const
{
    QRect rect = windowRect();
    QScreen *s = screen();
    if (!s)
        s = qGuiApp->primaryScreen();
    if (!s)
        return rect;

    const QRect screenGeometry = s->geometry();
    switch (m_settings->direction()) {
    case DockSettings::Left:
        rect.moveRight(screenGeometry.left() - 8);
        break;
    case DockSettings::Right:
        rect.moveLeft(screenGeometry.right() + 8);
        break;
    case DockSettings::Bottom:
    default:
        rect.moveTop(screenGeometry.bottom() + 8);
        break;
    }
    return rect;
}

void DockWindow::slideToHide()
{
    if (m_slideState == SlidingOut || (!isVisible() && m_slideState != SlidingIn))
        return;

    m_slideState = SlidingOut;
    m_slideAnimation->stop();
    m_slideAnimation->setDuration(280);
    m_slideAnimation->setEasingCurve(QEasingCurve::InQuint);
    m_slideAnimation->setStartValue(m_currentGeometry.isValid() ? m_currentGeometry : windowRect());
    m_slideAnimation->setEndValue(hiddenWindowRect());
    m_slideAnimation->start();
}

void DockWindow::slideToShow()
{
    if (m_slideState == SlidingIn || (isVisible() && m_slideState == Idle && m_currentGeometry == windowRect()))
        return;

    m_slideAnimation->stop();
    m_slideState = SlidingIn;

    if (!isVisible()) {
        setDockGeometry(hiddenWindowRect(), false);
        setVisible(true);
    }

    m_slideAnimation->setDuration(340);
    m_slideAnimation->setEasingCurve(QEasingCurve::OutQuint);
    m_slideAnimation->setStartValue(m_currentGeometry.isValid() ? m_currentGeometry : hiddenWindowRect());
    m_slideAnimation->setEndValue(windowRect());
    m_slideAnimation->start();
}

void DockWindow::updateMetrics()
{
    QScreen *s = screen();
    if (!s)
        s = qGuiApp->primaryScreen();
    if (!s)
        return;

    const bool isHorizontal = m_settings->direction() == DockSettings::Bottom;
    const int itemCount = m_appModel->rowCount() + 1;
    const int maxLength = (isHorizontal ? s->geometry().width()
                                        : s->availableGeometry().height())
            - m_settings->edgeMargins();

    DockMetrics metrics = DockMetrics::forIconSize(m_settings->iconSize());

    // Not enough room for every icon: shrink until the dock fits, keeping the
    // paddings in proportion with the icons.
    while (metrics.length(itemCount) > maxLength && metrics.iconSize > 1)
        metrics = DockMetrics::forIconSize(metrics.iconSize - 1);

    if (metrics == m_metrics)
        return;

    m_metrics = metrics;
    emit metricsChanged();
}

// Moving a plasma-shell surface takes more than QWindow::setGeometry(): the
// compositor places it at the position given to the shell surface, so every
// geometry change has to update both.
void DockWindow::applyGeometry()
{
    stopGeometryAnimation();
    updateMetrics();

    const QRect rect = windowRect();
    setDockGeometry(rect);
}

void DockWindow::animateToGeometry(const QRect &rect)
{
    if (m_currentGeometry == rect) {
        stopGeometryAnimation();
        return;
    }

    m_geometryAnimation->stop();
    m_geometryAnimation->setStartValue(m_currentGeometry);
    m_geometryAnimation->setEndValue(rect);
    setGeometryAnimationProgress(0.0);
    m_geometryAnimation->start();
}

void DockWindow::stopGeometryAnimation()
{
    if (m_geometryAnimation->state() != QAbstractAnimation::Stopped)
        m_geometryAnimation->stop();

    setGeometryAnimationProgress(1.0);
}

void DockWindow::setGeometryAnimationProgress(qreal progress)
{
    if (qFuzzyCompare(m_geometryAnimationProgress, progress))
        return;

    m_geometryAnimationProgress = progress;
    emit geometryAnimationProgressChanged();
}

void DockWindow::resizeWindow(bool animate)
{
    updateMetrics();

    const QRect rect = windowRect();
    if (animate) {
        animateToGeometry(rect);
    } else {
        stopGeometryAnimation();
        setDockGeometry(rect);
    }

    updateViewStruts();

    // The auto-hide trigger is a strip along the screen edge, so it has to
    // follow the screen as well.
    if (m_autoHideTrigger)
        m_autoHideTrigger->updateGeometry();

    emit resizingFished();
    checkWindowOverlap();
}

void DockWindow::setDockGeometry(const QRect &rect, bool requestRepaint)
{
    // QWindow::geometry() does not retain a reliable global position on
    // Wayland. Keep the requested geometry ourselves so a new animation can
    // start from the current animation frame rather than from (0, 0).
    if (m_currentGeometry == rect)
        return;

    m_currentGeometry = rect;
    setGeometry(rect);
    setSurfacePosition(rect.topLeft(), requestRepaint);
}

void DockWindow::updateViewStruts()
{
    QScreen *s = screen();
    if (!s)
        s = qGuiApp->primaryScreen();
    if (!s)
        return;

    const bool reserveSpace = m_settings->visibility() == DockSettings::AlwaysShow;

    // The reserved strip runs from the screen edge to the far side of the
    // dock, so it covers the dock plus its edge gap.
    const QRect screenRect = s->geometry();
    const QRect dockRect = windowRect();
    int thickness = 0;

    switch (m_settings->direction()) {
    case DockSettings::Left:
        m_strutSpacer->setEdge(Qt::LeftEdge);
        thickness = dockRect.right() - screenRect.left() + 1;
        break;
    case DockSettings::Right:
        m_strutSpacer->setEdge(Qt::RightEdge);
        thickness = screenRect.right() - dockRect.left() + 1;
        break;
    default:
        m_strutSpacer->setEdge(Qt::BottomEdge);
        thickness = screenRect.bottom() - dockRect.top() + 1;
        break;
    }

    m_strutSpacer->setScreen(s);
    m_strutSpacer->setThickness(reserveSpace ? thickness : 0);
}

void DockWindow::clearViewStruts()
{
    m_strutSpacer->setThickness(0);
}

void DockWindow::createAutoHideTrigger()
{
    if (!m_autoHideTrigger) {
        m_autoHideTrigger = new AutoHideTrigger(m_settings);
        m_autoHideTrigger->setScreen(screen());
        m_autoHideTrigger->updateGeometry();

        connect(m_autoHideTrigger, &AutoHideTrigger::containsMouseChanged, this, [=](bool contains) {
            m_mouseOverTrigger = contains;
            switch (m_settings->visibility()) {
            case DockSettings::AlwaysHide:
            case DockSettings::IntellHide:
            case DockSettings::AlwaysShow: {
                if (contains) {
                    m_hideTimer->stop();
                    m_hideBlocked = true;
                    slideToShow();
                } else {
                    if (!m_mouseOverDock && !m_menuOpen) {
                        m_hideBlocked = false;
                        m_hideTimer->start();
                    }
                }

                break;
            }
            default:
                break;
            }
        });
    }
}

void DockWindow::deleteAutoHideTrigger()
{
    if (m_autoHideTrigger) {
        m_mouseOverTrigger = false;
        disconnect(m_autoHideTrigger);
        m_autoHideTrigger->deleteLater();
        m_autoHideTrigger = nullptr;
    }
}

void DockWindow::onPositionChanged()
{
    // The window is moved in place, never unmapped and mapped again: a remap
    // reads to KWin as a window opening, which runs the open animation and the
    // startup feedback on it.
    applyGeometry();
    setVisible(true);
    updateViewStruts();

    emit directionChanged();
    checkWindowOverlap();
}

void DockWindow::onIconSizeChanged()
{
    applyGeometry();
    updateViewStruts();

    emit iconSizeChanged();
    checkWindowOverlap();
}

void DockWindow::checkWindowOverlap()
{
    bool touching = false;
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    if (kwin && kwin->available()) {
        const QRect dockRect = windowRect();
        const QRect screenRect = screen() ? screen()->geometry()
                                          : (qGuiApp->primaryScreen() ? qGuiApp->primaryScreen()->geometry() : QRect());
        touching = kwin->hasFullscreenWindow(screenRect) || kwin->hasWindowTouching(dockRect);
    }

    if (m_windowTouchingDock != touching) {
        m_windowTouchingDock = touching;
        onVisibilityChanged();
    }
}

void DockWindow::onVisibilityChanged()
{
    emit visibilityChanged();

    // The launcher covers the whole screen but leaves the reserved area
    // alone: clearing it would hand the full screen to every window opened
    // while the launcher is up.
    if (m_activity->launchPad()) {
        m_hideTimer->stop();
        setVisible(true);
        return;
    }

    // Visibility determination:
    // AlwaysHide: hides unless hovered.
    // IntellHide: hides when windows touch the dock or an app is fullscreen.
    // AlwaysShow: stays visible and reserves screen space.
    const bool shouldHide = (m_settings->visibility() == DockSettings::AlwaysHide)
                         || (m_settings->visibility() == DockSettings::IntellHide && m_windowTouchingDock);

    if (shouldHide) {
        clearViewStruts();

        if (!m_autoHideTrigger)
            createAutoHideTrigger();

        if (m_hideBlocked) {
            slideToShow();
        } else {
            slideToHide();
        }
    } else {
        // Not hidden: show and reserve space
        m_hideTimer->stop();
        slideToShow();
        updateViewStruts();

        if (m_settings->visibility() == DockSettings::AlwaysShow && m_autoHideTrigger)
            deleteAutoHideTrigger();
    }
}

void DockWindow::onHideTimeout()
{
    if (m_activity->launchPad() || m_menuOpen || m_mouseOverDock || m_mouseOverTrigger)
        return;

    const bool shouldHide = (m_settings->visibility() == DockSettings::AlwaysHide)
                         || (m_settings->visibility() == DockSettings::IntellHide && m_windowTouchingDock);

    if (shouldHide && !m_hideBlocked) {
        slideToHide();
    }
}

void DockWindow::exposeEvent(QExposeEvent *e)
{
    ShellSurface::exposeEvent(e);

    if (isExposed())
        emit minimizedGeometrySurfaceChanged();
}

bool DockWindow::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::Enter:
    case QEvent::MouseMove:
        m_mouseOverDock = true;
        m_hideTimer->stop();
        m_hideBlocked = true;
        if (!isVisible() || m_slideState == SlidingOut) {
            slideToShow();
        }
        break;
    case QEvent::Leave: {
        m_mouseOverDock = false;
        if (m_menuOpen || m_mouseOverTrigger) {
            m_hideBlocked = true;
            m_hideTimer->stop();
            break;
        }
        m_hideBlocked = false;
        m_hideTimer->start();
        break;
    }
    case QEvent::DragEnter:
    case QEvent::DragMove:
        m_hideTimer->stop();
        m_hideBlocked = true;
        if (!isVisible() || m_slideState == SlidingOut) {
            slideToShow();
        }
        break;
    case QEvent::DragLeave:
    case QEvent::Drop:
        if (!m_mouseOverDock && !m_mouseOverTrigger && !m_menuOpen) {
            m_hideBlocked = false;
            m_hideTimer->start();
        }
        break;
    default:
        break;
    }

    return QQuickWindow::eventFilter(obj, e);
}

void DockWindow::setMenuOpen(bool open)
{
    if (m_menuOpen != open) {
        m_menuOpen = open;
        if (m_menuOpen) {
            m_hideTimer->stop();
            m_hideBlocked = true;
            if (!isVisible() || m_slideState == SlidingOut) {
                slideToShow();
            }
        } else {
            if (!m_mouseOverDock && !m_mouseOverTrigger) {
                m_hideBlocked = false;
                m_hideTimer->start();
            }
        }
        emit menuOpenChanged();
    }
}

void DockWindow::resizeEvent(QResizeEvent *e)
{
    emit primaryGeometryChanged();

    // ShellSurface, not QQuickWindow: the dock's QML root has no size of its
    // own and has to follow the window.
    ShellSurface::resizeEvent(e);
}
