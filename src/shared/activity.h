#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <QObject>

class Activity : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(bool existsWindowMaximized READ existsWindowMaximized NOTIFY existsWindowMaximizedChanged)
    Q_PROPERTY(bool launchPad READ launchPad NOTIFY launchPadChanged)

public:
    static Activity *self();
    explicit Activity(QObject *parent = nullptr);

    bool existsWindowMaximized() const;

    /**
     * The launcher is a window of this very process now, so there is nothing to
     * infer from WM_CLASS any more: LauncherSurface reports its state directly.
     */
    void setLaunchPad(bool launchPad);

    bool launchPad() const;

    QString title() const;
    QString icon() const;

    Q_INVOKABLE void close();
    Q_INVOKABLE void minimize();
    Q_INVOKABLE void restore();
    Q_INVOKABLE void maximize();
    Q_INVOKABLE void toggleMaximize();
    Q_INVOKABLE void move();

    bool isAcceptableWindow(quint64 wid);

private slots:
    void onActiveWindowChanged();
    void updateExistsWindowMaximized();

    void clearTitle();
    void clearIcon();

signals:
    void titleChanged();
    void iconChanged();
    void existsWindowMaximizedChanged();
    void launchPadChanged();

private:
    QString m_title;
    QString m_icon;
    bool m_existsWindowMaximized;
    QString m_windowClass;
    quint32 m_pid;

    bool m_launchPad;
};

#endif // ACTIVITY_H
