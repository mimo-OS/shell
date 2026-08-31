/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     Kate Leet <kate@cutefishos.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NOTIFICATIONPOPUP_H
#define NOTIFICATIONPOPUP_H

#include <QQuickWindow>

struct wl_surface;

namespace KWayland
{
namespace Client
{
class PlasmaShellSurface;
}
}

/**
 * One on-screen notification banner.
 *
 * A QQuickWindow rather than a QQuickView: the contents are declared in
 * QmlNotificationPopup.qml, so the window never loads a source of its own and
 * a QQuickView would only add a second QQmlEngine to the shell process.
 *
 * The banner claims the org_kde_plasma_shell notification role, for the same
 * reason ShellSurface does it for the panels: a plain xdg-toplevel has no say
 * over where it lands, so the banners the QML lays out along the top right
 * corner used to end up centred and decorated instead.
 */
class NotificationPopup : public QQuickWindow
{
    Q_OBJECT
    //! Where the banner should sit, in global coordinates. It cannot be read
    //! back from x()/y(): Qt's Wayland plugin resets a toplevel's position to
    //! (0, 0) because plain xdg-shell has no global coordinates, so the QML
    //! that lays the banners out sets this instead.
    Q_PROPERTY(QPoint surfacePosition READ surfacePosition WRITE setSurfacePosition NOTIFY surfacePositionChanged)

public:
    explicit NotificationPopup(QQuickWindow *parent = nullptr);
    ~NotificationPopup() override;

    QPoint surfacePosition() const;
    void setSurfacePosition(const QPoint &position);

signals:
    void surfacePositionChanged();

protected:
    void exposeEvent(QExposeEvent *e) override;
    bool event(QEvent *e) override;

private:
    void applySurfaceRole();

    KWayland::Client::PlasmaShellSurface *m_plasmaSurface;
    wl_surface *m_wlSurface;
    QPoint m_surfacePosition;
};

#endif // NOTIFICATIONPOPUP_H
