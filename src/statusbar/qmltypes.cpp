#include "statusbar/qmltypes.h"

#include "statusbar/statusbarforegroundhelper.h"
#include "statusbar/battery.h"
#include "statusbar/brightness.h"
#include "statusbar/notifications.h"
#include "statusbar/poweractions.h"
#include "statusbar/appmenu/appmenumodel.h"
#include "statusbar/qmenu/qmenumodel.h"
#include "statusbar/systemtray/systemtraymodel.h"

#include <QQmlEngine>

void registerStatusBarQmlTypes()
{
    const char *uri = "Cutefish.StatusBar";

    qmlRegisterType<SystemTrayModel>(uri, 1, 0, "SystemTrayModel");
    qmlRegisterType<Brightness>(uri, 1, 0, "Brightness");
    qmlRegisterType<Battery>(uri, 1, 0, "Battery");
    qmlRegisterType<AppMenuModel>(uri, 1, 0, "AppMenuModel");
    qmlRegisterType<QMenuModel>(uri, 1, 0, "QMenuModel");
    qmlRegisterType<PowerActions>(uri, 1, 0, "PowerActions");
    qmlRegisterType<Notifications>(uri, 1, 0, "Notifications");
    qmlRegisterType<StatusBarForegroundHelper>(uri, 1, 0, "StatusBarForegroundHelper");
}
