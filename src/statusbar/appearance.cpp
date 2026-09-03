#include "appearance.h"

Appearance::Appearance(QObject *parent)
    : QObject(parent)
    , m_interface("com.cutefish.Services",
                  "/com/cutefish/Services/Appearance",
                  "com.cutefish.Services.Appearance",
                  QDBusConnection::sessionBus())
{
}

void Appearance::switchDarkMode(bool darkMode)
{
    m_interface.call("setDarkMode", darkMode);
}
