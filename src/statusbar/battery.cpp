#include "battery.h"
#include <QSettings>
#include <QDBusPendingCall>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTimer>

static const QString s_sServer = "com.cutefish.Services";
static const QString s_sPath = "/com/cutefish/Services/Battery";
static const QString s_sInterface = "com.cutefish.Services.Battery";

static Battery *SELF = nullptr;

static QString getBatteryPath()
{
    QDir dir("/sys/class/power_supply");
    QStringList bats = dir.entryList(QStringList() << "BAT*", QDir::Dirs);
    if (!bats.isEmpty()) {
        return "/sys/class/power_supply/" + bats.first();
    }
    return QString();
}

static int readSysfsInt(const QString &fileName)
{
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return f.readAll().trimmed().toInt();
    }
    return -1;
}

static QString readSysfsString(const QString &fileName)
{
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(f.readAll().trimmed());
    }
    return QString();
}

Battery *Battery::self()
{
    if (!SELF)
        SELF = new Battery;

    return SELF;
}

Battery::Battery(QObject *parent)
    : QObject(parent)
    , m_upowerInterface(new QDBusInterface("org.freedesktop.UPower",
                                           "/org/freedesktop/UPower",
                                           "org.freedesktop.UPower",
                                           QDBusConnection::systemBus(), this))
    , m_interface(nullptr)
    , m_available(false)
    , m_onBattery(false)
    , m_showPercentage(false)
    , m_powerMode(0)
    , m_autoSaverTriggered(false)
{
    QSettings settings("cutefishos", "statusbar");
    settings.setDefaultFormat(QSettings::IniFormat);
    m_showPercentage = settings.value("BatteryPercentage", false).toBool();

    QSettings pSettings("cutefishos", "power");
    pSettings.setDefaultFormat(QSettings::IniFormat);
    m_powerMode = pSettings.value("PowerMode", 0).toInt();

    QDBusConnection::systemBus().connect("org.freedesktop.UPower", "/org/freedesktop/UPower",
                                         "org.freedesktop.DBus.Properties",
                                         "PropertiesChanged", this,
                                         SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    if (m_upowerInterface && m_upowerInterface->isValid()) {
        m_onBattery = m_upowerInterface->property("OnBattery").toBool();
    }

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(s_sServer,
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                                           this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &Battery::initServices);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &) {
        bool hasHardware = !getBatteryPath().isEmpty();
        if (m_available != hasHardware) {
            m_available = hasHardware;
            emit validChanged();
        }
    });

    QTimer *fallbackTimer = new QTimer(this);
    fallbackTimer->setInterval(5000);
    connect(fallbackTimer, &QTimer::timeout, this, [this]() {
        if (!m_interface || !m_interface->isValid()) {
            emit chargePercentChanged(chargePercent());
            emit chargeStateChanged(chargeState());
            emit iconSourceChanged();
        }
    });
    fallbackTimer->start();

    initServices();
}

void Battery::initServices()
{
    delete m_interface;
    m_interface = new QDBusInterface(s_sServer, s_sPath, s_sInterface, QDBusConnection::sessionBus(), this);

    bool hasHardware = !getBatteryPath().isEmpty();
    bool newAvailable = (m_interface->isValid() && !m_interface->lastError().isValid()) || hasHardware;

    if (m_interface->isValid() && !m_interface->lastError().isValid()) {
        QDBusConnection::sessionBus().connect(s_sServer, s_sPath, s_sInterface, "chargeStateChanged", this, SLOT(chargeStateChanged(int)));
        QDBusConnection::sessionBus().connect(s_sServer, s_sPath, s_sInterface, "chargePercentChanged", this, SLOT(chargePercentChanged(int)));
        QDBusConnection::sessionBus().connect(s_sServer, s_sPath, s_sInterface, "lastChargedPercentChanged", this, SLOT(lastChargedPercentChanged()));
        QDBusConnection::sessionBus().connect(s_sServer, s_sPath, s_sInterface, "capacityChanged", this, SLOT(capacityChanged(int)));
        QDBusConnection::sessionBus().connect(s_sServer, s_sPath, s_sInterface, "remainingTimeChanged", this, SLOT(remainingTimeChanged(qlonglong)));
        QDBusConnection::sessionBus().connect(s_sServer, s_sPath, s_sInterface, "chargePercentChanged", this, SLOT(iconSourceChanged()));
    }

    if (m_available != newAvailable) {
        m_available = newAvailable;
        emit validChanged();
    }

    if (m_available) {
        emit chargeStateChanged(chargeState());
        emit chargePercentChanged(chargePercent());
        emit capacityChanged(capacity());
        emit iconSourceChanged();
    }
}

bool Battery::available() const
{
    return m_available;
}

bool Battery::onBattery() const
{
    return m_onBattery;
}

bool Battery::showPercentage() const
{
    return m_showPercentage;
}

void Battery::setShowPercentage(bool enabled)
{
    if (enabled == m_showPercentage)
        return;

    m_showPercentage = enabled;

    QSettings settings("cutefishos", "statusbar");
    settings.setDefaultFormat(QSettings::IniFormat);
    settings.setValue("BatteryPercentage", m_showPercentage);

    emit showPercentageChanged();
}

int Battery::powerMode() const
{
    return m_powerMode;
}

void Battery::setPowerMode(int mode)
{
    if (m_powerMode == mode)
        return;

    m_powerMode = mode;

    QSettings settings("cutefishos", "power");
    settings.setDefaultFormat(QSettings::IniFormat);
    settings.setValue("PowerMode", m_powerMode);

    QString profile = "balanced";
    QString cpuMode = "performance";
    if (m_powerMode == 1) {
        profile = "performance";
        cpuMode = "performance";
    } else if (m_powerMode == 2) {
        profile = "power-saver";
        cpuMode = "powersave";
    }

    QProcess::startDetached("powerprofilesctl", QStringList() << "set" << profile);
    QProcess::startDetached("cpupower", QStringList() << "frequency-set" << "-g" << cpuMode);

    emit powerModeChanged(m_powerMode);
}

int Battery::chargeState() const
{
    if (m_interface && m_interface->isValid())
        return m_interface->property("chargeState").toInt();

    QString bat = getBatteryPath();
    if (!bat.isEmpty()) {
        QString status = readSysfsString(bat + "/status");
        if (status.compare(QLatin1String("Charging"), Qt::CaseInsensitive) == 0) return 1;
        if (status.compare(QLatin1String("Discharging"), Qt::CaseInsensitive) == 0) return 2;
        if (status.compare(QLatin1String("Full"), Qt::CaseInsensitive) == 0) return 4;
    }
    return 0;
}

int Battery::chargePercent() const
{
    if (m_interface && m_interface->isValid())
        return m_interface->property("chargePercent").toInt();

    QString bat = getBatteryPath();
    if (!bat.isEmpty()) {
        int cap = readSysfsInt(bat + "/capacity");
        if (cap >= 0) return cap;
    }
    return 0;
}

int Battery::lastChargedPercent() const
{
    return (m_interface && m_interface->isValid()) ? m_interface->property("lastChargedPercent").toInt() : 0;
}

int Battery::capacity() const
{
    if (m_interface && m_interface->isValid())
        return m_interface->property("capacity").toInt();

    return chargePercent();
}

QString Battery::statusString() const
{
    return (m_interface && m_interface->isValid()) ? m_interface->property("statusString").toString() : QString();
}

QString Battery::iconSource() const
{
    int percent = this->chargePercent();
    int range = 0;

    if (percent >= 95)
        range = 100;
    else if (percent >= 85)
        range = 90;
    else if (percent>= 75)
        range = 80;
    else if (percent >= 65)
        range = 70;
    else if (percent >= 55)
        range = 60;
    else if (percent >= 45)
        range = 50;
    else if (percent >= 35)
        range = 40;
    else if (percent >= 25)
        range = 30;
    else if (percent >= 15)
        range = 20;
    else if (percent >= 5)
        range = 10;
    else
        range = 0;

    if (m_onBattery)
        return QString("battery-level-%1-symbolic.svg").arg(range);

    return QString("battery-level-%1-charging-symbolic.svg").arg(range);
}

void Battery::onPropertiesChanged(const QString &ifaceName, const QVariantMap &changedProps, const QStringList &invalidatedProps)
{
    Q_UNUSED(ifaceName);
    Q_UNUSED(invalidatedProps);

    bool onBattery = m_onBattery;
    if (changedProps.contains("OnBattery")) {
        onBattery = changedProps.value("OnBattery").toBool();
    } else if (m_upowerInterface && m_upowerInterface->isValid()) {
        onBattery = m_upowerInterface->property("OnBattery").toBool();
    }

    if (onBattery != m_onBattery) {
        m_onBattery = onBattery;
        if (m_interface && m_interface->isValid()) {
            m_interface->call("refresh");
        }
        emit onBatteryChanged();
        emit iconSourceChanged();
    }

    int currentPercent = chargePercent();
    if (m_onBattery && currentPercent > 0 && currentPercent <= 20 && m_powerMode != 2 && !m_autoSaverTriggered) {
        m_autoSaverTriggered = true;
        setPowerMode(2); // Smart Battery: Auto-enable Power Saver mode below 20%
    } else if (!m_onBattery || currentPercent > 25) {
        m_autoSaverTriggered = false;
    }
}
