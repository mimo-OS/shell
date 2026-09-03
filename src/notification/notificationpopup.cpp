#include "notification/notificationpopup.h"
#include "core/window/plasmasurface.h"

NotificationPopup::NotificationPopup(QQuickWindow *parent)
    : QQuickWindow(parent)
    , m_plasmaSurface(new PlasmaSurface(this))
{
    setColor(Qt::transparent);

    // The banners lay themselves out along a screen corner, so like the panels
    // they need a say in where the compositor puts them.
    m_plasmaSurface->setRole(PlasmaSurface::Role::Notification);
}

QPoint NotificationPopup::surfacePosition() const
{
    return m_plasmaSurface->position();
}

void NotificationPopup::setSurfacePosition(const QPoint &position)
{
    if (m_plasmaSurface->position() == position)
        return;

    // Still move the window itself: on X11 that is what places it, and the
    // blur and shadow behind it read the geometry back.
    setPosition(position);
    m_plasmaSurface->setPosition(position);

    emit surfacePositionChanged();
}
