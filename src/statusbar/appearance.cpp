#include "appearance.h"
#include "dock/docksettings.h"

Appearance::Appearance(QObject *parent)
    : QObject(parent)
    , m_interface("com.cutefish.Settings",
                  "/Theme",
                  "com.cutefish.Theme",
                  QDBusConnection::sessionBus())
    , m_dockSettings(DockSettings::self())
    , m_fontPointSize(11)
{
    connect(m_dockSettings, &DockSettings::iconSizeChanged, this, &Appearance::dockIconSizeChanged);
    connect(m_dockSettings, &DockSettings::directionChanged, this, &Appearance::dockDirectionChanged);

    if (m_interface.isValid()) {
        m_fontPointSize = m_interface.property("systemFontPointSize").toInt();
        connect(&m_interface, SIGNAL(darkModeDimsWallpaerChanged()), this, SIGNAL(dimsWallpaperChanged()));
    }
}

void Appearance::switchDarkMode(bool darkMode)
{
    m_interface.call("setDarkMode", darkMode);
}

bool Appearance::dimsWallpaper() const
{
    return m_interface.property("darkModeDimsWallpaer").toBool();
}

void Appearance::setDimsWallpaper(bool value)
{
    m_interface.call("setDarkModeDimsWallpaer", value);
}

int Appearance::dockIconSize() const
{
    return m_dockSettings->iconSize();
}

void Appearance::setDockIconSize(int dockIconSize)
{
    m_dockSettings->setIconSize(dockIconSize);
}

int Appearance::dockDirection() const
{
    return m_dockSettings->direction();
}

void Appearance::setDockDirection(int dockDirection)
{
    m_dockSettings->setDirection(static_cast<DockSettings::Direction>(dockDirection));
}

void Appearance::setGenericFontFamily(const QString &name)
{
    if (name.isEmpty())
        return;

    m_interface.call("setSystemFont", name);
}

void Appearance::setFixedFontFamily(const QString &name)
{
    if (name.isEmpty())
        return;

    m_interface.call("setSystemFixedFont", name);
}

int Appearance::fontPointSize() const
{
    return m_fontPointSize;
}

void Appearance::setFontPointSize(int fontPointSize)
{
    m_fontPointSize = fontPointSize;
    m_interface.call("setSystemFontPointSize", m_fontPointSize * 1.0);
}

void Appearance::setAccentColor(int accentColor)
{
    m_interface.call("setAccentColor", accentColor);
}
