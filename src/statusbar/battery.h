#ifndef BATTERY_H
#define BATTERY_H

#include <QObject>
#include <QDBusInterface>

class Battery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY validChanged)
    Q_PROPERTY(int chargeState READ chargeState NOTIFY chargeStateChanged)
    Q_PROPERTY(int chargePercent READ chargePercent NOTIFY chargePercentChanged)
    Q_PROPERTY(int lastChargedPercent READ lastChargedPercent NOTIFY lastChargedPercentChanged)
    Q_PROPERTY(int capacity READ capacity NOTIFY capacityChanged)
    Q_PROPERTY(QString statusString READ statusString NOTIFY remainingTimeChanged)
    Q_PROPERTY(bool onBattery READ onBattery NOTIFY onBatteryChanged)
    Q_PROPERTY(bool showPercentage READ showPercentage NOTIFY showPercentageChanged)
    Q_PROPERTY(QString iconSource READ iconSource NOTIFY iconSourceChanged)
    Q_PROPERTY(int powerMode READ powerMode WRITE setPowerMode NOTIFY powerModeChanged)

public:
    static Battery *self();
    explicit Battery(QObject *parent = nullptr);

    bool available() const;
    bool onBattery() const;

    bool showPercentage() const;
    void setShowPercentage(bool enabled);

    int powerMode() const;
    Q_INVOKABLE void setPowerMode(int mode);

    int chargeState() const;
    int chargePercent() const;
    int lastChargedPercent() const;
    int capacity() const;
    QString statusString() const;

    QString iconSource() const;

signals:
    void validChanged();
    void chargeStateChanged(int);
    void chargePercentChanged(int);
    void capacityChanged(int);
    void remainingTimeChanged(qlonglong time);
    void onBatteryChanged();
    void lastChargedPercentChanged();
    void iconSourceChanged();
    void showPercentageChanged();
    void powerModeChanged(int mode);

private slots:
    void onPropertiesChanged(const QString &ifaceName, const QVariantMap &changedProps, const QStringList &invalidatedProps);
    void initServices();

private:
    QDBusInterface *m_upowerInterface;
    QDBusInterface *m_interface;
    bool m_available;
    bool m_onBattery;
    bool m_showPercentage;
    int m_powerMode;
    bool m_autoSaverTriggered;
};

#endif // BATTERY_H
