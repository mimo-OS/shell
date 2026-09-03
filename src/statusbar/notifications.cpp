#include "notifications.h"
#include "notification/notificationsettings.h"

Notifications::Notifications(QObject *parent)
    : QObject(parent)
{
    connect(NotificationSettings::self(), &NotificationSettings::doNotDisturbChanged,
            this, &Notifications::doNotDisturbChanged);
}

bool Notifications::doNotDisturb() const
{
    return NotificationSettings::self()->doNotDisturb();
}

void Notifications::setDoNotDisturb(bool enabled)
{
    NotificationSettings::self()->setDoNotDisturb(enabled);
}
