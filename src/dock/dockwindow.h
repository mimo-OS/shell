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
    Q_PROPERTY(int visibility READ visibility WRITE setVisibility NOTIFY visibilityChanged)
    Q_PROPERTY(bool previewOpen READ previewOpen WRITE setPreviewOpen NOTIFY previewOpenChanged)
    Q_PROPERTY(QVariantMap metrics READ metrics NOTIFY metricsChanged)
    Q_PROPERTY(qreal geometryAnimationProgress READ geometryAnimationProgress
               NOTIFY geometryAnimationProgressChanged)
    Q_PROPERTY(bool geometryAnimationAdding READ geometryAnimationAdding
               NOTIFY geometryAnimationAddingChanged)
    Q_PROPERTY(int geometryAnimationToken READ geometryAnimationToken
               NOTIFY geometryAnimationTokenChanged)
    Q_PROPERTY(bool menuOpen READ isMenuOpen WRITE setMenuOpen NOTIFY menuOpenChanged)
    Q_PROPERTY(bool popupTipsOpen READ popupTipsOpen WRITE setPopupTipsOpen NOTIFY popupTipsOpenChanged)

public:
    explicit DockWindow(QQmlEngine *engine);
    ~DockWindow();

    bool isMenuOpen() const { return m_menuOpen; }
    void setMenuOpen(bool open);

    bool previewOpen() const { return m_previewOpen; }
    bool popupTipsOpen() const { return m_popupTipsOpen; }
    void setPopupTipsOpen(bool open);

    Q_INVOKABLE void setMouseOverDock(bool over);

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

    void setLauncher(Launcher *launcher);

    Q_INVOKABLE void updateIconGeometry(QQuickItem *item,
                                        const QStringList &windowIds);

public slots:
    void setDirection(int direction);
    void setIconSize(int iconSize);
    void setVisibility(int visibility);
    void setPreviewOpen(bool open);

signals:
    void resizingFished();
    void iconSizeChanged();
    void minimizedGeometrySurfaceChanged();
    void directionChanged();
    void primaryGeometryChanged();
    void visibilityChanged();
    void previewOpenChanged();
    void metricsChanged();
    void geometryAnimationProgressChanged();
    void geometryAnimationAddingChanged();
    void geometryAnimationTokenChanged();
    void menuOpenChanged();
    void popupTipsOpenChanged();

private:
    bool m_menuOpen = false;
    void updateMetrics();
    //! The dock's target geometry for the metrics computed last. Callers run
    //! updateMetrics() first.
    QRect windowRect() const;
    QRect hiddenWindowRect() const;
    QRect targetWindowRect() const;
    void slideToHide();
    void slideToShow();
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
    void checkWindowOverlap();

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

    bool isHovered() const;
    bool shouldHide() const;

    bool m_mouseOverDock = false;
    bool m_mouseOverTrigger = false;
    bool m_dragActive = false;
    bool m_previewOpen = false;
    bool m_popupTipsOpen = false;
    bool m_windowTouchingDock = false;

    enum SlideState {
        Idle,
        SlidingOut,
        SlidingIn
    };
    SlideState m_slideState = Idle;

    QTimer *m_showTimer;
    QTimer *m_hideTimer;
    QVariantAnimation *m_geometryAnimation;
    QVariantAnimation *m_slideAnimation;
    QRect m_currentGeometry;
    qreal m_geometryAnimationProgress = 1.0;
    bool m_geometryAnimationAdding = false;
    int m_geometryAnimationToken = 0;
};

#endif // DOCKWINDOW_H
