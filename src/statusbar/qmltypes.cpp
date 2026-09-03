#include "statusbar/qmltypes.h"

#include "statusbar/appearance.h"
#include "statusbar/backgroundhelper.h"
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
    qmlRegisterType<Appearance>(uri, 1, 0, "Appearance");
    qmlRegisterType<Brightness>(uri, 1, 0, "Brightness");
    qmlRegisterType<Battery>(uri, 1, 0, "Battery");
    qmlRegisterType<AppMenuModel>(uri, 1, 0, "AppMenuModel");
    qmlRegisterType<QMenuModel>(uri, 1, 0, "QMenuModel");
    qmlRegisterType<PowerActions>(uri, 1, 0, "PowerActions");
    qmlRegisterType<Notifications>(uri, 1, 0, "Notifications");
    qmlRegisterType<BackgroundHelper>(uri, 1, 0, "BackgroundHelper");
}
