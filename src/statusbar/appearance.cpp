#include "appearance.h"

Appearance::Appearance(QObject *parent)
    : QObject(parent)
    , m_interface("com.cutefish.Settings",
                  "/Theme",
                  "com.cutefish.Theme",
                  QDBusConnection::sessionBus())
{
}

void Appearance::switchDarkMode(bool darkMode)
{
    m_interface.call("setDarkMode", darkMode);
}
