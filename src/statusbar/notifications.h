#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QObject>

/**
 * The notification settings the control centre shows.
 *
 * The notification service runs in this same process, so this is a plain
 * binding onto NotificationSettings rather than a D-Bus proxy.
 */
class Notifications : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool doNotDisturb READ doNotDisturb WRITE setDoNotDisturb NOTIFY doNotDisturbChanged)

public:
    explicit Notifications(QObject *parent = nullptr);

    bool doNotDisturb() const;
    void setDoNotDisturb(bool enabled);

signals:
    void doNotDisturbChanged();
};

#endif // NOTIFICATIONS_H
