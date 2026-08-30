#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QGuiApplication>
#include <QTimer>

#include "shellsurface.h"

#include <QDBusInterface>

class Launcher : public ShellSurface
{
    Q_OBJECT
    Q_PROPERTY(QRect screenRect READ screenRect NOTIFY screenRectChanged)
    Q_PROPERTY(bool showed READ showed NOTIFY showedChanged)
    Q_PROPERTY(int leftMargin READ leftMargin NOTIFY marginsChanged)
    Q_PROPERTY(int rightMargin READ rightMargin NOTIFY marginsChanged)
    Q_PROPERTY(int bottomMargin READ bottomMargin NOTIFY marginsChanged)

public:
    Launcher(QQmlEngine *engine, bool firstShow = false);

    int leftMargin() const;
    int rightMargin() const;
    int bottomMargin() const;

    bool showed();

    Q_INVOKABLE void showWindow();
    Q_INVOKABLE void hideWindow();
    Q_INVOKABLE void toggle();

    Q_INVOKABLE bool dockAvailable();
    Q_INVOKABLE bool isPinedDock(const QString &desktop);

    Q_INVOKABLE void clearPixmapCache();

    QRect screenRect();

signals:
    void screenRectChanged();
    void showedChanged();
    void marginsChanged();

private slots:
    void updateMargins();
    void updateSize();
    void onGeometryChanged();

protected:
    void showEvent(QShowEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    void onActiveChanged();
    void focusWaylandWindow(quint64 address = 0);

private:
    QDBusInterface m_dockInterface;
    QRect m_screenRect;
    QTimer *m_hideTimer;
    bool m_showed;

    int m_leftMargin;
    int m_rightMargin;
    int m_bottomMargin;
};

#endif // LAUNCHER_H
