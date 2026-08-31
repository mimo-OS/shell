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

#include "notificationpopup.h"
#include "kwinwaylandinterface.h"

#include <QPlatformSurfaceEvent>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

using namespace KWayland::Client;

NotificationPopup::NotificationPopup(QQuickWindow *parent)
    : QQuickWindow(parent)
    , m_plasmaSurface(nullptr)
    , m_wlSurface(nullptr)
{
    setColor(Qt::transparent);
}

QPoint NotificationPopup::surfacePosition() const
{
    return m_surfacePosition;
}

void NotificationPopup::setSurfacePosition(const QPoint &position)
{
    if (m_surfacePosition == position)
        return;

    m_surfacePosition = position;

    // Still move the window itself: on X11 that is what places it, and the
    // blur and shadow behind it read the geometry back.
    setPosition(position);

    if (m_plasmaSurface)
        m_plasmaSurface->setPosition(position);

    emit surfacePositionChanged();
}

NotificationPopup::~NotificationPopup()
{
    delete m_plasmaSurface;
}

void NotificationPopup::applySurfaceRole()
{
    PlasmaShell *shell = KWinWaylandInterface::instance()->plasmaShell();
    if (!shell)
        return;

    // Qt drops the wl_surface whenever the window is hidden, and the banners
    // are hidden and shown all the time, so the plasma-shell surface has to be
    // rebuilt from the current one rather than kept for the window's lifetime.
    Surface *surface = Surface::fromWindow(this);
    wl_surface *native = surface ? static_cast<wl_surface *>(*surface) : nullptr;
    if (!native)
        return;

    if (native == m_wlSurface && m_plasmaSurface) {
        m_plasmaSurface->setPosition(m_surfacePosition);
        return;
    }

    delete m_plasmaSurface;
    m_wlSurface = native;
    m_plasmaSurface = shell->createSurface(surface, this);
    if (!m_plasmaSurface)
        return;

    m_plasmaSurface->setRole(PlasmaShellSurface::Role::Notification);
    m_plasmaSurface->setSkipTaskbar(true);
    m_plasmaSurface->setSkipSwitcher(true);
    m_plasmaSurface->setPosition(m_surfacePosition);
}

bool NotificationPopup::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        switch (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            // Claiming the role before anything is committed keeps KWin from
            // running its placement pass over the banner. See ShellSurface.
            applySurfaceRole();
            break;
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            delete m_plasmaSurface;
            m_plasmaSurface = nullptr;
            m_wlSurface = nullptr;
            break;
        }
    }

    return QQuickWindow::event(e);
}

void NotificationPopup::exposeEvent(QExposeEvent *e)
{
    QQuickWindow::exposeEvent(e);

    if (isExposed())
        applySurfaceRole();
}
