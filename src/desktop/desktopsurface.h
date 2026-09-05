#ifndef DESKTOPSURFACE_H
#define DESKTOPSURFACE_H

#include <QPointer>

#include "core/window/shellsurface.h"

/**
 * The desktop window for one screen.
 *
 * This class deliberately knows nothing about files. It owns the window -- its
 * geometry and stacking -- and its QML draws the background. The icons, layout,
 * selection, drag and drop, menus and every file operation come from the
 * DesktopView item of "org.cutefish.filemanager.desktop", which the QML puts on
 * top of the background on the primary screen.
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

protected:
    void exposeEvent(QExposeEvent *event) override;

private slots:
    void updateGeometry();

private:
    void claimKeyboard();

    QPointer<QScreen> m_screen;
    bool m_primary;
    bool m_keyboardClaimed;
};

#endif // DESKTOPSURFACE_H
