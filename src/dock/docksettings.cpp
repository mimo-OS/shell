#include "docksettings.h"
#include "dockmetrics.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDBusInterface>

#include <QFile>
#include <QDebug>

static DockSettings *SELF = nullptr;

DockSettings *DockSettings::self()
{
    if (SELF == nullptr)
        SELF = new DockSettings;

    return SELF;
}

DockSettings::DockSettings(QObject *parent)
    : QObject(parent)
    , m_iconSize(0)
    , m_edgeMargins(0)
    , m_roundedWindowEnabled(true)
    , m_direction(Left)
    , m_visibility(AlwaysShow)
    , m_settings(new QSettings(QSettings::UserScope, "cutefishos", "dock", this))
{
    if (!m_settings->contains("IconSize"))
        m_settings->setValue("IconSize", DockMetrics::baseIconSize);
    if (!m_settings->contains("Direction"))
        m_settings->setValue("Direction", Bottom);
    if (!m_settings->contains("Visibility"))
        m_settings->setValue("Visibility", AlwaysShow);
    if (!m_settings->contains("RoundedWindow"))
        m_settings->setValue("RoundedWindow", true);
    if (!m_settings->contains("EdgeMargins"))
        m_settings->setValue("EdgeMargins", 10);

    // Full-width dock style was removed. Keep old configurations centered.
    m_settings->setValue("Style", 0);

    m_settings->sync();

    m_iconSize = m_settings->value("IconSize").toInt();
    m_direction = static_cast<Direction>(m_settings->value("Direction").toInt());
    m_visibility = static_cast<Visibility>(m_settings->value("Visibility", AlwaysShow).toInt());
    m_roundedWindowEnabled = m_settings->value("RoundedWindow").toBool();
    m_edgeMargins = m_settings->value("EdgeMargins").toInt();
}

int DockSettings::iconSize() const
{
    return m_iconSize;
}

void DockSettings::setIconSize(int iconSize)
{
    if (m_iconSize != iconSize) {
        m_iconSize = iconSize;
        m_settings->setValue("IconSize", iconSize);
        emit iconSizeChanged();
    }
}

DockSettings::Direction DockSettings::direction() const
{
    return m_direction;
}

void DockSettings::setDirection(const Direction &direction)
{
    if (m_direction != direction) {
        m_direction = direction;
        m_settings->setValue("Direction", direction);
        emit directionChanged();
    }
}

DockSettings::Visibility DockSettings::visibility() const
{
    return m_visibility;
}

void DockSettings::setVisibility(const DockSettings::Visibility &visibility)
{
    if (m_visibility != visibility) {
        m_visibility = visibility;
        m_settings->setValue("Visibility", visibility);
        emit visibilityChanged();
    }
}

int DockSettings::edgeMargins() const
{
    return m_edgeMargins;
}

void DockSettings::setEdgeMargins(int edgeMargins)
{
    m_edgeMargins = edgeMargins;
}

bool DockSettings::roundedWindowEnabled() const
{
    return m_roundedWindowEnabled;
}

void DockSettings::setRoundedWindowEnabled(bool enabled)
{
    if (m_roundedWindowEnabled != enabled) {
        m_roundedWindowEnabled = enabled;
        emit roundedWindowEnabledChanged();
    }
}
