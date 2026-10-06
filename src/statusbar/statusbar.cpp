#include "statusbar.h"
#include "battery.h"
#include "appmenu/appmenu.h"
#include "statusbaradaptor.h"

#include <QQmlEngine>
#include <QQmlContext>

#include <QDBusConnection>
#include <QSettings>
#include <QScreen>
#include <QAction>
#include <QProcess>
#include <KGlobalAccel>

StatusBar::StatusBar(QQmlEngine *engine)
    : ShellSurface(engine)
    , m_acticity(Activity::self())
{
    m_strutSpacer = nullptr;

    QSettings settings("cutefishos", "locale");
    m_twentyFourTime = settings.value("twentyFour", false).toBool();

    setFlags(Qt::FramelessWindowHint);
    setTitle(QStringLiteral("Status Bar"));
    setColor(Qt::transparent);

    new StatusbarAdaptor(this);
    new AppMenu(this);
    setContextProperty("StatusBar", this);
    setContextProperty("acticity", m_acticity);
    setContextProperty("battery", Battery::self());

    setSource(QUrl(QStringLiteral("qrc:/statusbar/qml/main.qml")));
    configureSurfaceRole(SurfaceRole::Panel);

    // The status bar always lives on the primary screen.
    followPrimaryScreen([this] { updateGeometry(); });

    setVisible(true);
    initState();
    setupGlobalShortcuts();

    connect(m_acticity, &Activity::launchPadChanged, this, &StatusBar::initState);
}

QRect StatusBar::screenRect()
{
    return m_screenRect;
}

bool StatusBar::twentyFourTime()
{
    return m_twentyFourTime;
}

void StatusBar::setBatteryPercentage(bool enabled)
{
    Battery::self()->setShowPercentage(enabled);
}

void StatusBar::setTwentyFourTime(bool t)
{
    if (m_twentyFourTime != t) {
        m_twentyFourTime = t;
        emit twentyFourTimeChanged();
    }
}

void StatusBar::toggleSpotlight()
{
    emit spotlightRequested();
}

void StatusBar::updateGeometry()
{
    QScreen *s = screen();
    if (!s)
        s = qGuiApp->primaryScreen();
    if (!s)
        return;

    const QRect rect = s->geometry();

    if (m_screenRect != rect) {
        m_screenRect = rect;
        emit screenRectChanged();
    }

    const QRect windowRect(rect.x(), rect.y(), rect.width(), Height);
    setGeometry(windowRect);
    setSurfacePosition(windowRect.topLeft());
    updateViewStruts();
}

void StatusBar::updateViewStruts()
{
    QScreen *s = screen();
    if (!s)
        s = qGuiApp->primaryScreen();
    if (!s)
        return;

    // A plasma-shell panel does not reserve work area on its own; the
    // spacer does it. See StrutSpacer.
    if (!m_strutSpacer)
        m_strutSpacer = new StrutSpacer(this);

    m_strutSpacer->setEdge(Qt::TopEdge);
    m_strutSpacer->setScreen(s);
    m_strutSpacer->setThickness(geometry().height());
}

void StatusBar::initState()
{
    // The launcher is an ordinary full-screen window, so the status bar has
    // to step out of its way.
    setPanelBehavior(m_acticity->launchPad() ? PanelBehavior::AllowCovering
                                             : PanelBehavior::StayAbove);
    updateViewStruts();
}

void StatusBar::setupGlobalShortcuts()
{
    // Spotlight search shortcut: Meta+Space and Ctrl+Space
    QAction *spotlightAction = new QAction(this);
    spotlightAction->setObjectName(QStringLiteral("SpotlightSearch"));
    spotlightAction->setText(tr("Spotlight Search"));
    KGlobalAccel::self()->setGlobalShortcut(spotlightAction, QList<QKeySequence>() << QKeySequence(Qt::META | Qt::Key_Space) << QKeySequence(Qt::CTRL | Qt::Key_Space));
    connect(spotlightAction, &QAction::triggered, this, &StatusBar::toggleSpotlight);

    // Screenshot Area shortcut: Shift+Meta+4 and Ctrl+Alt+A
    QAction *screenshotAreaAction = new QAction(this);
    screenshotAreaAction->setObjectName(QStringLiteral("ScreenshotArea"));
    screenshotAreaAction->setText(tr("Take Screenshot Area"));
    KGlobalAccel::self()->setGlobalShortcut(screenshotAreaAction, QList<QKeySequence>() << QKeySequence(Qt::SHIFT | Qt::META | Qt::Key_4) << QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A));
    connect(screenshotAreaAction, &QAction::triggered, this, []() {
        QProcess::startDetached(QStringLiteral("cutefish-screenshot"), QStringList() << QStringLiteral("-a"));
    });

    // Screenshot Fullscreen shortcut: Shift+Meta+3 and Print
    QAction *screenshotFullAction = new QAction(this);
    screenshotFullAction->setObjectName(QStringLiteral("ScreenshotFull"));
    screenshotFullAction->setText(tr("Take Fullscreen Screenshot"));
    KGlobalAccel::self()->setGlobalShortcut(screenshotFullAction, QList<QKeySequence>() << QKeySequence(Qt::SHIFT | Qt::META | Qt::Key_3) << QKeySequence(Qt::Key_Print));
    connect(screenshotFullAction, &QAction::triggered, this, []() {
        QProcess::startDetached(QStringLiteral("cutefish-screenshot"), QStringList());
    });
}

