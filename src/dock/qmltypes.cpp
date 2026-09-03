#include "dock/qmltypes.h"
#include "dock/docksettings.h"

#include <QQmlEngine>

void registerDockQmlTypes()
{
    // Uncreatable: the QML only reads the Direction enum off the type, and the
    // one instance the dock follows is exposed as the "Settings" context
    // property.
    qmlRegisterUncreatableType<DockSettings>("Cutefish.Dock", 1, 0, "DockSettings",
                                             QStringLiteral("Use the Settings context property."));
}
