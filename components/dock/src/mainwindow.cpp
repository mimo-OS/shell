#include "mainwindow.h"
#include "processprovider.h"
#include "dockadaptor.h"

#include <QGuiApplication>
#include <QScreen>

#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QMetaEnum>

#include <NETWM>
#include <KX11Extras>
#include <KWindowEffects>
#include <KWindowSystem>

MainWindow::MainWindow(QQmlEngine *engine)
    : ShellSurface(engine)
    , m_activity(Activity::self())
    , m_settings(DockSettings::self())
    , m_appModel(new ApplicationModel)
    , m_fakeWindow(nullptr)
    , m_strutSpacer(new StrutSpacer(this))
    , m_trashManager(new TrashManager)
    , m_hideBlocked(false)
    , m_showTimer(new QTimer(this))
    , m_hideTimer(new QTimer(this))
{
    new DockAdaptor(this);

    installEventFilter(this);

    setDefaultAlphaBuffer(false);
    setColor(Qt::transparent);

    // On X11 the dock is a non-focusable EWMH panel.  A Wayland xdg-toplevel
    // still needs to accept pointer focus, otherwise Qt Quick receives a
    // mapped surface that the compositor will not route clicks to.
    setFlags(Qt::FramelessWindowHint);
    if (KWindowSystem::isPlatformX11())
        setFlag(Qt::WindowDoesNotAcceptFocus);
    setTitle(QStringLiteral("Dock"));
    if (KWindowSystem::isPlatformX11())
        KX11Extras::setType(winId(), NET::Dock);

    setContextProperty("appModel", m_appModel);
    setContextProperty("process", new ProcessProvider(this));
    setContextProperty("Settings", m_settings);
    setContextProperty("mainWindow", this);
    setContextProperty("trash", m_trashManager);

    setSource(QUrl(QStringLiteral("qrc:/dock/qml/main.qml")));
    setScreen(qApp->primaryScreen());
    initScreens();
    configureSurfaceRole(SurfaceRole::Dock);

    initSlideWindow();
    resizeWindow();
    onVisibilityChanged();

    m_showTimer->setSingleShot(true);
    m_showTimer->setInterval(200);
    connect(m_showTimer, &QTimer::timeout, this, [=] { setVisible(true); });

    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(500);
    connect(m_hideTimer, &QTimer::timeout, this, &MainWindow::onHideTimeout);

    // When the current window changes.
    connect(m_activity, &Activity::launchPadChanged, this, &MainWindow::onVisibilityChanged);
    connect(m_activity, &Activity::existsWindowMaximizedChanged, this, &MainWindow::onVisibilityChanged);

    // Screen change.
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &MainWindow::onPrimaryScreenChanged);
    connect(screen(), &QScreen::virtualGeometryChanged, this, &MainWindow::resizeWindow);
    connect(screen(), &QScreen::geometryChanged, this, &MainWindow::resizeWindow);

    connect(m_appModel, &ApplicationModel::countChanged, this, &MainWindow::resizeWindow);
    connect(m_settings, &DockSettings::directionChanged, this, &MainWindow::onPositionChanged);
    connect(m_settings, &DockSettings::iconSizeChanged, this, &MainWindow::onIconSizeChanged);
    connect(m_settings, &DockSettings::visibilityChanged, this, &MainWindow::onVisibilityChanged);
}

MainWindow::~MainWindow()
{
}

void MainWindow::add(const QString &desktop)
{
    m_appModel->addItem(desktop);
}

void MainWindow::remove(const QString &desktop)
{
    m_appModel->removeItem(desktop);
}

bool MainWindow::pinned(const QString &desktop)
{
    return m_appModel->isDesktopPinned(desktop);
}

QRect MainWindow::primaryGeometry() const
{
    return geometry();
}

int MainWindow::direction() const
{
    return DockSettings::self()->direction();
}

int MainWindow::visibility() const
{
    return DockSettings::self()->visibility();
}

void MainWindow::setDirection(int direction)
{
    DockSettings::self()->setDirection(static_cast<DockSettings::Direction>(direction));
}

void MainWindow::setIconSize(int iconSize)
{
    DockSettings::self()->setIconSize(iconSize);
}

void MainWindow::setVisibility(int visibility)
{
    DockSettings::self()->setVisibility(static_cast<DockSettings::Visibility>(visibility));
}

void MainWindow::updateSize()
{
    resizeWindow();
}

QRect MainWindow::windowRect() const
{
    const QRect screenGeometry = screen()->geometry();
    const QRect availableGeometry = screen()->availableGeometry();

    bool isHorizontal = m_settings->direction() == DockSettings::Bottom;
    bool compositing = false;
    QQuickItem *item = rootItem();

    if (item) {
        compositing = item->property("compositing").toBool();
    }

    QSize newSize(0, 0);
    QPoint position(0, 0);
    int maxLength = isHorizontal ? screenGeometry.width() - m_settings->edgeMargins()
                                 : availableGeometry.height() - m_settings->edgeMargins();;

    struct DockMetrics {
        int sidePadding;
        int topPadding;
        int iconGap;
        int iconDotGap;
        int dotSize;
        int bottomPadding;
    };

    const auto metricsForScale = [](qreal scale) {
        DockMetrics metrics;
        metrics.sidePadding = qMax(1, qRound(16 * scale));
        metrics.topPadding = qMax(1, qRound(16 * scale));
        metrics.iconGap = qMax(1, qRound(16 * scale));
        metrics.iconDotGap = qMax(1, qRound(8 * scale));
        metrics.dotSize = qMax(1, qRound(4 * scale));
        metrics.bottomPadding = qMax(1, qRound(4 * scale));
        return metrics;
    };

    // Add trash item.
    const int appCount = m_appModel->rowCount() + 1;
    const auto lengthForIconSize = [isHorizontal, appCount](int iconSize,
                                                             const DockMetrics &metrics) {
        const int iconGaps = qMax(0, appCount - 1) * metrics.iconGap;
        if (isHorizontal) {
            return metrics.sidePadding * 2 + appCount * iconSize + iconGaps;
        }

        return metrics.topPadding + metrics.bottomPadding + appCount * iconSize + iconGaps;
    };

    const qreal sizeScale = m_settings->iconSize() / 53.0;
    int iconSize = qMax(1, qRound(54 * sizeScale));
    DockMetrics metrics = metricsForScale(sizeScale);
    int length = lengthForIconSize(iconSize, metrics);
    int margins = compositing ? DockSettings::self()->edgeMargins() / 2 : 0;

    while (length > maxLength && iconSize > 1) {
        --iconSize;
        metrics = metricsForScale(iconSize / 54.0);
        length = lengthForIconSize(iconSize, metrics);
    }

    switch (m_settings->direction()) {
    case DockSettings::Left:
        newSize = QSize(metrics.sidePadding * 2 + iconSize, length);
        position.setX(screenGeometry.x() + margins);
        // Handle the top statusbar.
        position.setY(availableGeometry.y() + (availableGeometry.height() - newSize.height()) / 2);
        break;
    case DockSettings::Bottom:
        newSize = QSize(length,
                        metrics.topPadding + iconSize + metrics.iconDotGap
                        + metrics.dotSize + metrics.bottomPadding);
        position.setX(screenGeometry.x() + (screenGeometry.width() - newSize.width()) / 2);
        position.setY(screenGeometry.y() + screenGeometry.height() - newSize.height() - margins);
        break;
    case DockSettings::Right:
        newSize = QSize(metrics.sidePadding * 2 + iconSize, length);
        position.setX(screenGeometry.x() + screenGeometry.width() - newSize.width() - margins);
        position.setY(availableGeometry.y() + (availableGeometry.height() - newSize.height()) / 2);
        break;
    default:
        break;
    }

    return QRect(position, newSize);
}

void MainWindow::resizeWindow()
{
    const QRect rect = windowRect();
    setGeometry(rect);
    setSurfacePosition(rect.topLeft());
    updateViewStruts();

    emit resizingFished();
}

void MainWindow::initScreens()
{
    switch (m_settings->direction()) {
// TODO
//    case DockSettings::Left:
//        setScreen(qGuiApp->screens().first());
//        break;
//    case DockSettings::Right:
//        setScreen(qGuiApp->screens().last());
//        break;
//    case DockSettings::Bottom:
//        setScreen(qGuiApp->primaryScreen());
//        break;
    default:
        setScreen(qGuiApp->primaryScreen());
        break;
    }

    if (m_fakeWindow) {
        m_fakeWindow->setScreen(screen());
        m_fakeWindow->updateGeometry();
    }
}

void MainWindow::initSlideWindow()
{
    if (!KWindowSystem::isPlatformX11())
        return;

    KWindowEffects::SlideFromLocation location = KWindowEffects::NoEdge;

    if (m_settings->direction() == DockSettings::Left)
        location = KWindowEffects::LeftEdge;
    else if (m_settings->direction() == DockSettings::Right)
        location = KWindowEffects::RightEdge;
    else if (m_settings->direction() == DockSettings::Bottom)
        location = KWindowEffects::BottomEdge;

    KWindowEffects::slideWindow(this, location);
}

void MainWindow::updateViewStruts()
{
    bool compositing = false;
    QQuickItem *item = rootItem();

    if (item) {
        compositing = item->property("compositing").toBool();
    }

    const bool reserveSpace = m_settings->visibility() == DockSettings::AlwaysShow
        && !m_activity->launchPad();

    if (!KWindowSystem::isPlatformX11()) {
        // The reserved strip runs from the screen edge to the far side of the
        // dock, so it covers the dock plus the gap it floats above the edge
        // with. See StrutSpacer for why a panel cannot claim this itself.
        const QRect screenRect = screen()->geometry();
        // Not geometry(): Qt reports every Wayland toplevel at (0, 0), so the
        // dock's real placement only exists in windowRect().
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

        m_strutSpacer->setScreen(screen());
        m_strutSpacer->setThickness(reserveSpace ? thickness : 0);
        return;
    }

    if (reserveSpace || m_activity->launchPad()) {
        XWindowInterface::instance()->setViewStruts(this, m_settings->direction(), geometry(), compositing);
    } else {
        clearViewStruts();
    }
}

void MainWindow::clearViewStruts()
{
    if (!KWindowSystem::isPlatformX11()) {
        m_strutSpacer->setThickness(0);
        return;
    }

    XWindowInterface::instance()->clearViewStruts(this);
}

void MainWindow::createFakeWindow()
{
    if (!m_fakeWindow) {
        m_fakeWindow = new FakeWindow;
        m_fakeWindow->setScreen(screen());
        m_fakeWindow->updateGeometry();

        connect(m_fakeWindow, &FakeWindow::containsMouseChanged, this, [=](bool contains) {
            switch (m_settings->visibility()) {
            case DockSettings::AlwaysHide:
            case DockSettings::IntellHide:{
                if (contains) {
                    m_hideTimer->stop();

                    // reionwong: The mouse is moved to fakewindow,
                    // if the dock is not displayed,
                    // it will start to display.
                    if (!isVisible() && !m_showTimer->isActive()) {
                        m_showTimer->start();
                    }
                } else {
                    if (!m_hideBlocked)
                        m_hideTimer->start();
                }

                break;
            }
            default:
                break;
            }
        });
    }
}

void MainWindow::deleteFakeWindow()
{
    if (m_fakeWindow) {
        // removeEventFilter(this);
        disconnect(m_fakeWindow);
        m_fakeWindow->deleteLater();
        m_fakeWindow = nullptr;
    }
}

void MainWindow::onPrimaryScreenChanged(QScreen *screen)
{
    initScreens();
    setScreen(screen);
    resizeWindow();
}

void MainWindow::onPositionChanged()
{
    initScreens();

    if (m_settings->visibility() == DockSettings::AlwaysHide ||
            m_settings->visibility() == DockSettings::IntellHide) {
        setVisible(false);
        initSlideWindow();
        // Setting geometry needs to be displayed, otherwise it will be invalid.
        setVisible(true);
        setGeometry(windowRect());
        updateViewStruts();

        m_hideTimer->start();
    } else if (m_settings->visibility() == DockSettings::AlwaysShow) {
        setVisible(false);
        initSlideWindow();
        setVisible(true);
        setGeometry(windowRect());
        updateViewStruts();
    }

    emit directionChanged();
}

void MainWindow::onIconSizeChanged()
{
    const QRect rect = windowRect();
    setGeometry(rect);
    setSurfacePosition(rect.topLeft());
    updateViewStruts();

    emit iconSizeChanged();
}

void MainWindow::onVisibilityChanged()
{
    emit visibilityChanged();

    if (m_activity->launchPad()) {
        m_hideTimer->stop();
        clearViewStruts();
        setVisible(true);
        return;
    }

    // Always show
    // Must remain displayed when launchpad is opened.
    if (m_settings->visibility() == DockSettings::AlwaysShow) {
        m_hideTimer->stop();

        setGeometry(windowRect());
        setVisible(true);
        updateViewStruts();

        // Delete fakewindow
        if (m_fakeWindow) {
            deleteFakeWindow();
        }
    }

    if (m_settings->visibility() == DockSettings::IntellHide) {
        clearViewStruts();
        setGeometry(windowRect());

        if (m_activity->existsWindowMaximized() && !m_hideBlocked) {
            setVisible(false);
        } else {
            setVisible(true);
        }

        if (!m_fakeWindow)
            createFakeWindow();
    }

    // Always hide
    if (m_settings->visibility() == DockSettings::AlwaysHide) {
        clearViewStruts();
        setGeometry(windowRect());
        setVisible(m_hideBlocked);

        // Create
        if (!m_fakeWindow)
            createFakeWindow();
    }
}

void MainWindow::onHideTimeout()
{
    if (m_activity->launchPad())
        return;

    if (m_settings->visibility() == DockSettings::IntellHide
            && !m_activity->existsWindowMaximized()) {
        return;
    }

    setVisible(false);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::Enter:
        if (m_fakeWindow)
            m_hideTimer->stop();
        m_hideBlocked = true;
        break;
    case QEvent::Leave:
        if (m_fakeWindow)
            m_hideTimer->start();
        m_hideBlocked = false;
        break;
    case QEvent::DragEnter:
    case QEvent::DragMove:
        if (m_fakeWindow)
            m_hideTimer->stop();
        break;
    case QEvent::DragLeave:
    case QEvent::Drop:
        if (m_fakeWindow)
            m_hideTimer->stop();
        break;
    default:
        break;
    }

    return QQuickWindow::eventFilter(obj, e);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    emit primaryGeometryChanged();

    // ShellSurface, not QQuickWindow: the dock's QML root has no size of its
    // own and has to follow the window.
    ShellSurface::resizeEvent(e);
}
