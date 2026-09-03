#include "notification/qmltypes.h"

#include "notification/historymodel.h"
#include "notification/notificationpopup.h"
#include "notification/notificationsmodel.h"

#include <QQmlEngine>

void registerNotificationQmlTypes()
{
    const char *uri = "Cutefish.Notification";

    qmlRegisterType<NotificationsModel>(uri, 1, 0, "NotificationsModel");
    qmlRegisterType<HistoryModel>(uri, 1, 0, "HistoryModel");
    qmlRegisterType<NotificationPopup>(uri, 1, 0, "NotificationPopup");
}
