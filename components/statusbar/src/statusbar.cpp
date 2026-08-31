#include "statusbar.h"
#include "battery.h"
#include "processprovider.h"
#include "appmenu/appmenu.h"
#include "statusbaradaptor.h"

#include <QQmlEngine>
#include <QQmlContext>

#include <QDBusConnection>
#include <QApplication>
#include <QSettings>
#include <QScreen>

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
    setContextProperty("process", new ProcessProvider(this));
    setContextProperty("battery", Battery::self());

    setSource(QUrl(QStringLiteral("qrc:/statusbar/qml/main.qml")));
    setScreen(qApp->primaryScreen());
    configureSurfaceRole(SurfaceRole::Panel);
    updateGeometry();
    setVisible(true);
    initState();

    connect(m_acticity, &Activity::launchPadChanged, this, &StatusBar::initState);

    connect(screen(), &QScreen::virtualGeometryChanged, this, &StatusBar::updateGeometry);
    connect(screen(), &QScreen::geometryChanged, this, &StatusBar::updateGeometry);

    // Always show on the main screen
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &StatusBar::onPrimaryScreenChanged);
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

void StatusBar::updateGeometry()
{
    const QRect rect = screen()->geometry();

    if (m_screenRect != rect) {
        m_screenRect = rect;
        emit screenRectChanged();
    }

    QRect windowRect = QRect(rect.x(), rect.y(), rect.width(), 25);
    setGeometry(windowRect);
    setSurfacePosition(windowRect.topLeft());
    updateViewStruts();

}

void StatusBar::updateViewStruts()
{
    // A plasma-shell panel does not reserve work area on its own; the
    // spacer does it. See StrutSpacer.
    if (!m_strutSpacer)
        m_strutSpacer = new StrutSpacer(this);

    m_strutSpacer->setEdge(Qt::TopEdge);
    m_strutSpacer->setScreen(screen());
    m_strutSpacer->setThickness(m_acticity->launchPad() ? 0 : geometry().height());
}

void StatusBar::initState()
{
    // The launcher is an ordinary full-screen window, so the status bar has
    // to step out of its way. It also stops reserving work area while the
    // launcher is up.
    setPanelBehavior(m_acticity->launchPad() ? PanelBehavior::AllowCovering
                                             : PanelBehavior::StayAbove);
    updateViewStruts();
}

void StatusBar::onPrimaryScreenChanged(QScreen *screen)
{
    disconnect(this->screen());

    setScreen(screen);
    updateGeometry();

    connect(screen, &QScreen::virtualGeometryChanged, this, &StatusBar::updateGeometry);
    connect(screen, &QScreen::geometryChanged, this, &StatusBar::updateGeometry);
}
