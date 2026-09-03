#include "dbusmenu_interface.h"

/*
 * Implementation of interface class DBusMenuInterface
 */

DBusMenuInterface::DBusMenuInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

DBusMenuInterface::~DBusMenuInterface()
{
}
