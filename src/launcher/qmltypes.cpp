#include "launcher/qmltypes.h"

#include "launcher/appmanager.h"
#include "launcher/iconitem.h"
#include "launcher/launchermodel.h"
#include "launcher/pagemodel.h"

#include <QAbstractItemModel>
#include <QQmlEngine>

void registerLauncherQmlTypes()
{
    const char *uri = "Cutefish.Launcher";

    qmlRegisterType<LauncherModel>(uri, 1, 0, "LauncherModel");
    qmlRegisterType<PageModel>(uri, 1, 0, "PageModel");
    qmlRegisterType<IconItem>(uri, 1, 0, "IconItem");
    qmlRegisterType<AppManager>(uri, 1, 0, "AppManager");
    qmlRegisterAnonymousType<QAbstractItemModel>(uri, 0);
}
