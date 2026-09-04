#ifndef STATUSBARFOREGROUNDHELPER_H
#define STATUSBARFOREGROUNDHELPER_H

#include <QObject>
#include <QColor>
#include <QPointer>

class QScreen;
class QTimer;

/**
 * Picks light or dark status bar contents from what is behind the bar: the
 * top strip of the wallpaper, or the plain background colour.
 */
class StatusBarForegroundHelper : public QObject
{
    Q_OBJECT
    // valid is false until a background has been sampled, and whenever
    // sampling fails; the bar falls back to the system dark mode then.
    Q_PROPERTY(bool valid READ isValid NOTIFY foregroundChanged)
    Q_PROPERTY(bool useLightForeground READ useLightForeground NOTIFY foregroundChanged)

public:
    explicit StatusBarForegroundHelper(QObject *parent = nullptr);

    bool isValid() const;
    bool useLightForeground() const;

    Q_INVOKABLE void setColor(const QColor &color);
    Q_INVOKABLE void setBackground(const QString &fileName, bool dimmed);
    Q_INVOKABLE void setUnavailable();

signals:
    void foregroundChanged();

private slots:
    void onPrimaryScreenChanged();
    void onScreenChanged();

private:
    enum class Type {
        Unavailable,
        Color,
        Wallpaper,
    };

    void startWallpaperSampling();
    void setForeground(bool valid, bool useLight);

    QPointer<QScreen> m_screen;
    QTimer *m_sampleTimer;
    int m_statusBarHeight;
    Type m_type;
    QColor m_color;
    QString m_wallpaper;
    bool m_dimmed;
    quint64 m_requestSerial;
    bool m_valid;
    bool m_useLightForeground;
};

#endif // STATUSBARFOREGROUNDHELPER_H
