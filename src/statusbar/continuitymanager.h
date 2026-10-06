#ifndef CONTINUITYMANAGER_H
#define CONTINUITYMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>

class ContinuityManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY statusChanged)
    Q_PROPERTY(bool isPaired READ isPaired NOTIFY statusChanged)
    Q_PROPERTY(bool isReachable READ isReachable NOTIFY statusChanged)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY statusChanged)
    Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY statusChanged)
    Q_PROPERTY(QString deviceType READ deviceType NOTIFY statusChanged)
    Q_PROPERTY(int batteryCharge READ batteryCharge NOTIFY batteryChanged)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY batteryChanged)
    Q_PROPERTY(int signalStrength READ signalStrength NOTIFY statusChanged)
    Q_PROPERTY(QString networkType READ networkType NOTIFY statusChanged)
    Q_PROPERTY(bool clipboardSync READ clipboardSync WRITE setClipboardSync NOTIFY settingsChanged)
    Q_PROPERTY(bool notificationSync READ notificationSync WRITE setNotificationSync NOTIFY settingsChanged)
    Q_PROPERTY(bool showInMenuBar READ showInMenuBar WRITE setShowInMenuBar NOTIFY settingsChanged)
    Q_PROPERTY(bool kdeConnectActive READ isKdeConnectActive NOTIFY backendChanged)

public:
    explicit ContinuityManager(QObject *parent = nullptr);
    ~ContinuityManager() override = default;

    bool isConnected() const;
    bool isPaired() const;
    bool isReachable() const;
    QString deviceId() const;
    QString deviceName() const;
    QString deviceType() const;
    int batteryCharge() const;
    bool isCharging() const;
    int signalStrength() const;
    QString networkType() const;
    bool clipboardSync() const;
    bool notificationSync() const;
    bool showInMenuBar() const;
    bool isKdeConnectActive() const;

    Q_INVOKABLE void ringPhone();
    Q_INVOKABLE void sendPing();
    Q_INVOKABLE void shareFile(const QString &filePath);
    Q_INVOKABLE void shareText(const QString &text);
    Q_INVOKABLE void toggleConnect();
    Q_INVOKABLE void setDeviceName(const QString &name);
    Q_INVOKABLE void setClipboardSync(bool enabled);
    Q_INVOKABLE void setNotificationSync(bool enabled);
    Q_INVOKABLE void setShowInMenuBar(bool enabled);
    Q_INVOKABLE void openKdeConnect();
    Q_INVOKABLE void refresh();

signals:
    void statusChanged();
    void batteryChanged();
    void settingsChanged();
    void backendChanged();

private slots:
    void onServiceRegistered(const QString &service);
    void onServiceUnregistered(const QString &service);
    void queryKdeConnectDevices();
    void onDeviceReachableChanged(bool reachable);
    void onBatteryChargeChanged(int charge);
    void onBatteryChargingChanged(bool charging);

private:
    void loadLocalSettings();
    void saveLocalSettings();

    bool m_kdeConnectActive = false;
    QString m_deviceId;
    QString m_deviceName;
    QString m_deviceType = QStringLiteral("phone");
    bool m_connected = false;
    bool m_paired = false;
    bool m_reachable = false;
    int m_batteryCharge = -1;
    bool m_isCharging = false;
    int m_signalStrength = 0;
    QString m_networkType;

    bool m_clipboardSync = true;
    bool m_notificationSync = true;
    bool m_showInMenuBar = true;

    QDBusServiceWatcher *m_serviceWatcher = nullptr;
};

#endif // CONTINUITYMANAGER_H
