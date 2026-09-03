#ifndef APPEARANCE_H
#define APPEARANCE_H

#include <QObject>
#include <QDBusInterface>

/**
 * The appearance controls of the control centre.
 *
 * Theme settings belong to cutefish-settings and are changed over D-Bus. Its
 * own settings pages drive the rest of them (fonts, accent colour, and the dock
 * through com.cutefish.Dock) from their own copy of this class; anything the
 * control centre grows that touches the dock should use DockSettings instead,
 * which the dock in this process follows directly.
 */
class Appearance : public QObject
{
    Q_OBJECT

public:
    explicit Appearance(QObject *parent = nullptr);

    Q_INVOKABLE void switchDarkMode(bool darkMode);

private:
    QDBusInterface m_interface;
};

#endif // APPEARANCE_H
