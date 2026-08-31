#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>

#include "shellsurface.h"

#include "activity.h"
#include "docksettings.h"
#include "applicationmodel.h"
#include "fakewindow.h"
#include "trashmanager.h"
#include "strutspacer.h"
#include "dockmetrics.h"

class MainWindow : public ShellSurface
{
    Q_OBJECT
    Q_PROPERTY(QRect primaryGeometry READ primaryGeometry NOTIFY primaryGeometryChanged)
    Q_PROPERTY(int direction READ direction NOTIFY directionChanged)
    Q_PROPERTY(int visibility READ visibility NOTIFY visibilityChanged)
    Q_PROPERTY(QVariantMap metrics READ metrics NOTIFY metricsChanged)

public:
    explicit MainWindow(QQmlEngine *engine);
    ~MainWindow();

    // DBus interface
    void add(const QString &desktop);
    void remove(const QString &desktop);
    bool pinned(const QString &desktop);

    QRect primaryGeometry() const;
    int direction() const;

    int visibility() const;
    QVariantMap metrics() const;

    void setDirection(int direction);
    void setIconSize(int iconSize);
    void setVisibility(int visibility);

signals:
    void resizingFished();
    void iconSizeChanged();
    void directionChanged();
    void primaryGeometryChanged();
    void visibilityChanged();
    void metricsChanged();

private:
    DockMetrics metricsFor(int itemCount, int maxLength) const;
    void updateMetrics();
    QRect windowRect() const;
    void applyGeometry();
    void resizeWindow();
    void initScreens();
    void updateViewStruts();
    void clearViewStruts();

    void createFakeWindow();
    void deleteFakeWindow();

private slots:
    void onPrimaryScreenChanged(QScreen *screen);
    void onPositionChanged();
    void onIconSizeChanged();
    void onVisibilityChanged();

    void onHideTimeout();

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;
    void resizeEvent(QResizeEvent *) override;

private:
    Activity *m_activity;
    DockSettings *m_settings;
    DockMetrics m_metrics;
    ApplicationModel *m_appModel;
    FakeWindow *m_fakeWindow;
    StrutSpacer *m_strutSpacer;
    TrashManager *m_trashManager;

    bool m_hideBlocked;

    QTimer *m_showTimer;
    QTimer *m_hideTimer;
};

#endif // MAINWINDOW_H
