#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QTimer>

#include "core/window/shellsurface.h"

class DockWindow;

class Launcher : public ShellSurface
{
    Q_OBJECT
    Q_PROPERTY(QRect screenRect READ screenRect NOTIFY screenRectChanged)
    Q_PROPERTY(bool showed READ showed NOTIFY showedChanged)
    Q_PROPERTY(int topMargin READ topMargin NOTIFY marginsChanged)
    Q_PROPERTY(int leftMargin READ leftMargin NOTIFY marginsChanged)
    Q_PROPERTY(int rightMargin READ rightMargin NOTIFY marginsChanged)
    Q_PROPERTY(int bottomMargin READ bottomMargin NOTIFY marginsChanged)

public:
    Launcher(QQmlEngine *engine, bool firstShow = false);

    int topMargin() const;
    int leftMargin() const;
    int rightMargin() const;
    int bottomMargin() const;

    bool showed();

    void setDock(DockWindow *dock);

    /**
     * D-Bus entry points, called by the generated adaptor.
     *
     * The adaptor calls the slot named like the D-Bus method, and QWindow
     * already has show() and hide(): without these, com.cutefish.Launcher.show
     * mapped the window straight from QWindow, behind the launcher's back. The
     * panels never gave their reserved area back, so KWin sized the launcher to
     * the work area and it came up a strip short at the bottom, unfocused, and
     * with the dock still stacked over it.
     */
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();

    Q_INVOKABLE void showWindow();
    Q_INVOKABLE void hideWindow();
    Q_INVOKABLE void toggle();

    Q_INVOKABLE bool dockAvailable();
    Q_INVOKABLE bool isPinedDock(const QString &desktop);
    Q_INVOKABLE void sendToDock(const QString &desktop);
    Q_INVOKABLE void removeFromDock(const QString &desktop);

    Q_INVOKABLE void clearPixmapCache();

    /**
     * A window-level drag hands the pointer to the compositor, which can cost
     * this window its activation. Without this the launcher would hide itself
     * out from under the icon being dragged.
     */
    Q_INVOKABLE void setDragging(bool dragging);

    QRect screenRect();

signals:
    void screenRectChanged();
    void showedChanged();
    void marginsChanged();

private slots:
    void updateMargins();
    void updateSize();

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void onActiveChanged();
    void focusWaylandWindow(quint64 address = 0);

private:
    DockWindow *m_dock;
    QRect m_screenRect;
    QTimer *m_hideTimer;
    bool m_showed;
    bool m_dragging;

    int m_topMargin;
    int m_leftMargin;
    int m_rightMargin;
    int m_bottomMargin;
};

#endif // LAUNCHER_H
