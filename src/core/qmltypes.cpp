#include "core/qmltypes.h"
#include "core/services/screenhelper.h"
#include "core/window/popupwindow.h"

#include <QQmlEngine>

void registerCoreQmlTypes()
{
    const char *uri = "Cutefish.Shell";

    // The work area is shell-wide: on Wayland only the shell knows it, because
    // it is the shell that reserves the struts.
    qmlRegisterType<ScreenHelper>(uri, 1, 0, "ScreenHelper");
    qmlRegisterType<PopupWindow>(uri, 1, 0, "PopupWindow");
}
