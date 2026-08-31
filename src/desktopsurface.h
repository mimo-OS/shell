#ifndef DESKTOPSURFACE_H
#define DESKTOPSURFACE_H

#include <QPointer>

#include "shellsurface.h"

/**
 * The desktop window for one screen.
 *
 * This class deliberately knows nothing about files. It owns the window -- its
 * geometry and stacking -- and hands the screen it belongs to
 * over to the DesktopView item from "org.cutefish.filemanager.desktop", which
 * is where the wallpaper, icons, layout, selection, drag and drop, menus and
 * every file operation actually live.
 */
class DesktopSurface : public ShellSurface
{
    Q_OBJECT
    Q_PROPERTY(QScreen *targetScreen READ targetScreen NOTIFY targetScreenChanged)
    Q_PROPERTY(bool primary READ isPrimary NOTIFY primaryChanged)

public:
    DesktopSurface(QQmlEngine *engine, QScreen *screen);

    QScreen *targetScreen() const;

    bool isPrimary() const;
    void setPrimary(bool primary);

signals:
    void targetScreenChanged();
    void primaryChanged();

private slots:
    void updateGeometry();

private:
    QPointer<QScreen> m_screen;
    bool m_primary;
};

#endif // DESKTOPSURFACE_H
