#ifndef CUTEFISH_PLASMASURFACE_H
#define CUTEFISH_PLASMASURFACE_H

#include <QObject>
#include <QPoint>

#include <KWayland/Client/plasmashell.h>

class QWindow;
struct wl_surface;

/**
 * Keeps an org_kde_plasma_shell surface attached to a QWindow.
 *
 * Every window the shell maps -- the panels, the desktop, the launcher, the
 * notification banners, the dock's auto-hide trigger -- needs the same thing:
 * a role, a stacking behaviour and a global position that plain xdg-shell
 * cannot express. Qt destroys the wl_surface whenever the window is hidden, so
 * the attachment is rebuilt from the window's current surface every time one
 * appears; this class watches the window's events for that and needs no
 * cooperation from the window itself.
 *
 * Nothing is sent to the compositor until setRole() has been called.
 */
class PlasmaSurface : public QObject
{
    Q_OBJECT

public:
    using Role = KWayland::Client::PlasmaShellSurface::Role;
    using PanelBehavior = KWayland::Client::PlasmaShellSurface::PanelBehavior;

    explicit PlasmaSurface(QWindow *window);
    ~PlasmaSurface() override;

    void setRole(Role role);
    void setPanelBehavior(PanelBehavior behavior);
    void setPanelTakesFocus(bool takesFocus);

    //! Where the compositor should put the surface, in global coordinates. It
    //! cannot be read back from QWindow::geometry(): Qt's Wayland plugin
    //! resets a toplevel's position to (0, 0) because plain xdg-shell has no
    //! global coordinates.
    void setPosition(const QPoint &position);
    QPoint position() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void attach();
    void detach();

    QWindow *m_window;
    KWayland::Client::PlasmaShellSurface *m_surface = nullptr;
    wl_surface *m_wlSurface = nullptr;
    QPoint m_position;
    Role m_role = Role::Normal;
    PanelBehavior m_panelBehavior = PanelBehavior::WindowsGoBelow;
    bool m_hasRole = false;
    bool m_panelTakesFocus = true;
};

#endif // CUTEFISH_PLASMASURFACE_H
