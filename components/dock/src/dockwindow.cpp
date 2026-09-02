#include "dockwindow.h"
#include "processprovider.h"
#include "dockadaptor.h"

#include <QGuiApplication>
#include <QScreen>

#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QMetaEnum>
#include <QQuickItem>
#include <QExposeEvent>

#include <KWayland/Client/surface.h>

#include "kwinwaylandinterface.h"

DockWindow::DockWindow(QQmlEngine *engine)
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

    setFlags(Qt::FramelessWindowHint);
    setTitle(QStringLiteral("Dock"));

    setContextProperty("appModel", m_appModel);
    setContextProperty("process", new ProcessProvider(this));
    setContextProperty("Settings", m_settings);
    setContextProperty("dock", this);
    setContextProperty("trash", m_trashManager);

    setScreen(qApp->primaryScreen());
    updateMetrics();
    setSource(QUrl(QStringLiteral("qrc:/dock/qml/main.qml")));
    initScreens();
    configureSurfaceRole(SurfaceRole::Dock);

    resizeWindow();
    onVisibilityChanged();

    m_showTimer->setSingleShot(true);
    m_showTimer->setInterval(200);
    connect(m_showTimer, &QTimer::timeout, this, [=] { setVisible(true); });

    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(500);
    connect(m_hideTimer, &QTimer::timeout, this, &DockWindow::onHideTimeout);

    // When the current window changes.
    connect(m_activity, &Activity::launchPadChanged, this, &DockWindow::onVisibilityChanged);
    connect(m_activity, &Activity::existsWindowMaximizedChanged, this, &DockWindow::onVisibilityChanged);

    // Screen change.
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &DockWindow::onPrimaryScreenChanged);
    watchScreen(screen(), [this] { resizeWindow(); });

    connect(m_appModel, &ApplicationModel::countChanged, this, &DockWindow::resizeWindow);
    connect(m_settings, &DockSettings::directionChanged, this, &DockWindow::onPositionChanged);
    connect(m_settings, &DockSettings::iconSizeChanged, this, &DockWindow::onIconSizeChanged);
    connect(m_settings, &DockSettings::visibilityChanged, this, &DockWindow::onVisibilityChanged);
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

DockMetrics DockWindow::metricsFor(int itemCount, int maxLength) const
{
    DockMetrics metrics = DockMetrics::forIconSize(m_settings->iconSize());

    // Not enough room for every icon: shrink until the dock fits, keeping the
    // paddings in proportion with the icons.
    while (metrics.length(itemCount) > maxLength && metrics.iconSize > 1)
        metrics = DockMetrics::forIconSize(metrics.iconSize - 1);

    return metrics;
}

QVariantMap DockWindow::metrics() const
{
    return m_metrics.toVariantMap();
}

QRect DockWindow::windowRect() const
{
    const QRect screenGeometry = screen()->geometry();
    const QRect availableGeometry = screen()->availableGeometry();

    const bool isHorizontal = m_settings->direction() == DockSettings::Bottom;

    // Add the trash item.
    const int itemCount = m_appModel->rowCount() + 1;
    const int margins = m_settings->edgeMargins() / 2;
    const int maxLength = (isHorizontal ? screenGeometry.width()
                                        : availableGeometry.height())
            - m_settings->edgeMargins();

    const DockMetrics metrics = metricsFor(itemCount, maxLength);
    const int length = metrics.length(itemCount);
    const int thickness = metrics.thickness();

    QSize newSize(0, 0);
    QPoint position(0, 0);

    switch (m_settings->direction()) {
    case DockSettings::Left:
        newSize = QSize(thickness, length);
        position.setX(screenGeometry.x() + margins);
        // Handle the top statusbar.
        position.setY(availableGeometry.y() + (availableGeometry.height() - newSize.height()) / 2);
        break;
    case DockSettings::Bottom:
        newSize = QSize(length, thickness);
        position.setX(screenGeometry.x() + (screenGeometry.width() - newSize.width()) / 2);
        position.setY(screenGeometry.y() + screenGeometry.height() - newSize.height() - margins);
        break;
    case DockSettings::Right:
        newSize = QSize(thickness, length);
        position.setX(screenGeometry.x() + screenGeometry.width() - newSize.width() - margins);
        position.setY(availableGeometry.y() + (availableGeometry.height() - newSize.height()) / 2);
        break;
    default:
        break;
    }

    return QRect(position, newSize);
}

void DockWindow::updateMetrics()
{
    const bool isHorizontal = m_settings->direction() == DockSettings::Bottom;
    const int itemCount = m_appModel->rowCount() + 1;
    const int maxLength = (isHorizontal ? screen()->geometry().width()
                                        : screen()->availableGeometry().height())
            - m_settings->edgeMargins();

    const DockMetrics metrics = metricsFor(itemCount, maxLength);
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
    updateMetrics();

    const QRect rect = windowRect();
    setGeometry(rect);
    setSurfacePosition(rect.topLeft());
}

void DockWindow::resizeWindow()
{
    applyGeometry();
    updateViewStruts();

    // The auto-hide trigger is a strip along the screen edge, so it has to
    // follow the screen as well.
    if (m_fakeWindow)
        m_fakeWindow->updateGeometry();

    emit resizingFished();
}

void DockWindow::initScreens()
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

void DockWindow::updateViewStruts()
{
    const bool reserveSpace = m_settings->visibility() == DockSettings::AlwaysShow
        && !m_activity->launchPad();

    // The reserved strip runs from the screen edge to the far side of the
    // dock, so it covers the dock plus its edge gap.
    const QRect screenRect = screen()->geometry();
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
}

void DockWindow::clearViewStruts()
{
    m_strutSpacer->setThickness(0);
}

void DockWindow::createFakeWindow()
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

void DockWindow::deleteFakeWindow()
{
    if (m_fakeWindow) {
        // removeEventFilter(this);
        disconnect(m_fakeWindow);
        m_fakeWindow->deleteLater();
        m_fakeWindow = nullptr;
    }
}

void DockWindow::onPrimaryScreenChanged(QScreen *screen)
{
    Q_UNUSED(screen)

    // initScreens() is what picks the screen the dock belongs on.
    initScreens();
    watchScreen(this->screen(), [this] { resizeWindow(); });
    resizeWindow();
}

void DockWindow::onPositionChanged()
{
    initScreens();

    // The window is moved in place, never unmapped and mapped again: a remap
    // reads to KWin as a window opening, which runs the open animation and the
    // startup feedback on it.
    applyGeometry();
    setVisible(true);
    updateViewStruts();

    if (m_settings->visibility() == DockSettings::AlwaysHide ||
            m_settings->visibility() == DockSettings::IntellHide) {
        m_hideTimer->start();
    }

    emit directionChanged();
}

void DockWindow::onIconSizeChanged()
{
    applyGeometry();
    updateViewStruts();

    emit iconSizeChanged();
}

void DockWindow::onVisibilityChanged()
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

        applyGeometry();
        setVisible(true);
        updateViewStruts();

        // Delete fakewindow
        if (m_fakeWindow) {
            deleteFakeWindow();
        }
    }

    if (m_settings->visibility() == DockSettings::IntellHide) {
        clearViewStruts();
        applyGeometry();

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
        applyGeometry();
        setVisible(m_hideBlocked);

        // Create
        if (!m_fakeWindow)
            createFakeWindow();
    }
}

void DockWindow::onHideTimeout()
{
    if (m_activity->launchPad())
        return;

    if (m_settings->visibility() == DockSettings::IntellHide
            && !m_activity->existsWindowMaximized()) {
        return;
    }

    setVisible(false);
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

void DockWindow::resizeEvent(QResizeEvent *e)
{
    emit primaryGeometryChanged();

    // ShellSurface, not QQuickWindow: the dock's QML root has no size of its
    // own and has to follow the window.
    ShellSurface::resizeEvent(e);
}
