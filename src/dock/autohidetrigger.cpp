#include "dock/autohidetrigger.h"
#include "dock/docksettings.h"
#include "core/window/plasmasurface.h"

#include <QGuiApplication>
#include <QScreen>

AutoHideTrigger::AutoHideTrigger(DockSettings *settings, QWindow *parent)
    : QQuickWindow(parent)
    , m_settings(settings)
    , m_plasmaSurface(new PlasmaSurface(this))
    , m_delayedContainsMouse(false)
    , m_containsMouse(false)
{
    setColor(Qt::transparent);
    setDefaultAlphaBuffer(true);
    setFlags(Qt::FramelessWindowHint |
             Qt::WindowStaysOnTopHint |
             Qt::NoDropShadowWindowHint |
             Qt::WindowDoesNotAcceptFocus);
    // Not shown here: the window is mapped by updateGeometry() once it knows
    // where the screen edge is. Showing it beforehand maps it at the default
    // size, which swallows the pointer and pops the dock straight back out.

    m_plasmaSurface->setPanelTakesFocus(false);
    m_plasmaSurface->setRole(PlasmaSurface::Role::Panel);
    m_plasmaSurface->setPanelBehavior(PlasmaSurface::PanelBehavior::WindowsGoBelow);

    m_delayedMouseTimer.setSingleShot(true);
    m_delayedMouseTimer.setInterval(50);
    connect(&m_delayedMouseTimer, &QTimer::timeout, this,
            [this] { setContainsMouse(m_delayedContainsMouse); });

    connect(m_settings, &DockSettings::directionChanged, this, &AutoHideTrigger::updateGeometry);
}

bool AutoHideTrigger::containsMouse() const
{
    return m_containsMouse;
}

bool AutoHideTrigger::event(QEvent *e)
{
    if (e->type() == QEvent::DragEnter || e->type() == QEvent::DragMove) {
        if (!m_containsMouse) {
            m_delayedContainsMouse = false;
            m_delayedMouseTimer.stop();
            setContainsMouse(true);
            emit dragEntered();
        }
    } else if (e->type() == QEvent::Enter) {
        m_delayedContainsMouse = true;
        if (!m_delayedMouseTimer.isActive())
            m_delayedMouseTimer.start();
    } else if (e->type() == QEvent::Leave || e->type() == QEvent::DragLeave) {
        m_delayedContainsMouse = false;
        if (!m_delayedMouseTimer.isActive())
            m_delayedMouseTimer.start();
    }

    return QQuickWindow::event(e);
}

void AutoHideTrigger::setContainsMouse(bool contains)
{
    if (m_containsMouse == contains)
        return;

    m_containsMouse = contains;
    emit containsMouseChanged(contains);
}

void AutoHideTrigger::updateGeometry()
{
    const int length = 5;
    const QRect screenRect = screen() ? screen()->geometry()
                                      : qGuiApp->primaryScreen()->geometry();
    QRect newRect;

    switch (m_settings->direction()) {
    case DockSettings::Left:
        newRect = QRect(screenRect.x(), screenRect.y(), length, screenRect.height());
        break;
    case DockSettings::Bottom:
        newRect = QRect(screenRect.x(), screenRect.y() + screenRect.height() - length,
                        screenRect.width(), length);
        break;
    case DockSettings::Right:
        newRect = QRect(screenRect.x() + screenRect.width() - length, screenRect.y(),
                        length, screenRect.height());
        break;
    }

    setGeometry(newRect);
    m_plasmaSurface->setPosition(newRect.topLeft());

    if (!isVisible())
        show();
}
