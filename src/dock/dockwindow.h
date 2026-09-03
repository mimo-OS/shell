#ifndef DOCKWINDOW_H
#define DOCKWINDOW_H

#include <QTimer>
#include <QStringList>
#include <QVariantAnimation>

#include "core/window/shellsurface.h"

#include "core/services/activity.h"
#include "docksettings.h"
#include "applicationmodel.h"
#include "dock/autohidetrigger.h"
#include "trashmanager.h"
#include "core/wayland/strutspacer.h"
#include "dockmetrics.h"

class Launcher;

class DockWindow : public ShellSurface
{
    Q_OBJECT
    Q_PROPERTY(QRect primaryGeometry READ primaryGeometry NOTIFY primaryGeometryChanged)
    Q_PROPERTY(int direction READ direction NOTIFY directionChanged)
    Q_PROPERTY(int visibility READ visibility NOTIFY visibilityChanged)
    Q_PROPERTY(QVariantMap metrics READ metrics NOTIFY metricsChanged)
    Q_PROPERTY(qreal geometryAnimationProgress READ geometryAnimationProgress
               NOTIFY geometryAnimationProgressChanged)
    Q_PROPERTY(bool geometryAnimationAdding READ geometryAnimationAdding
               NOTIFY geometryAnimationAddingChanged)
    Q_PROPERTY(int geometryAnimationToken READ geometryAnimationToken
               NOTIFY geometryAnimationTokenChanged)

public:
    explicit DockWindow(QQmlEngine *engine);
    ~DockWindow();

    // DBus interface
    void add(const QString &desktop);
    void remove(const QString &desktop);
    bool pinned(const QString &desktop);

    QRect primaryGeometry() const;
    int direction() const;

    int visibility() const;
    QVariantMap metrics() const;
    qreal geometryAnimationProgress() const;
    bool geometryAnimationAdding() const;
    int geometryAnimationToken() const;

    void setDirection(int direction);
    void setIconSize(int iconSize);
    void setVisibility(int visibility);
    void setLauncher(Launcher *launcher);

    Q_INVOKABLE void updateIconGeometry(QQuickItem *item,
                                        const QStringList &windowIds);

signals:
    void resizingFished();
    void iconSizeChanged();
    void minimizedGeometrySurfaceChanged();
    void directionChanged();
    void primaryGeometryChanged();
    void visibilityChanged();
    void metricsChanged();
    void geometryAnimationProgressChanged();
    void geometryAnimationAddingChanged();
    void geometryAnimationTokenChanged();

private:
    void updateMetrics();
    //! The dock's target geometry for the metrics computed last. Callers run
    //! updateMetrics() first.
    QRect windowRect() const;
    void animateToGeometry(const QRect &rect);
    void stopGeometryAnimation();
    void setGeometryAnimationProgress(qreal progress);
    void setDockGeometry(const QRect &rect, bool requestRepaint = true);
    void applyGeometry();
    void resizeWindow(bool animate = false);
    void updateViewStruts();
    void clearViewStruts();

    void createAutoHideTrigger();
    void deleteAutoHideTrigger();

private slots:
    void onPositionChanged();
    void onIconSizeChanged();
    void onVisibilityChanged();

    void onHideTimeout();

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;
    void exposeEvent(QExposeEvent *e) override;
    void resizeEvent(QResizeEvent *) override;

private:
    Activity *m_activity;
    DockSettings *m_settings;
    DockMetrics m_metrics;
    ApplicationModel *m_appModel;
    AutoHideTrigger *m_autoHideTrigger;
    StrutSpacer *m_strutSpacer;
    TrashManager *m_trashManager;

    bool m_hideBlocked;

    QTimer *m_showTimer;
    QTimer *m_hideTimer;
    QVariantAnimation *m_geometryAnimation;
    QRect m_currentGeometry;
    qreal m_geometryAnimationProgress = 1.0;
    bool m_geometryAnimationAdding = false;
    int m_geometryAnimationToken = 0;
};

#endif // DOCKWINDOW_H
