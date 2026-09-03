#ifndef APPEARANCE_H
#define APPEARANCE_H

#include <QObject>
#include <QDBusInterface>

class DockSettings;

/**
 * The appearance controls of the control centre.
 *
 * Theme, fonts and accent colour belong to cutefish-settings and are set over
 * D-Bus; the dock properties are read from and written to the shell's own
 * DockSettings, which is what the dock in this process actually follows.
 */
class Appearance : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int dockIconSize READ dockIconSize WRITE setDockIconSize NOTIFY dockIconSizeChanged)
    Q_PROPERTY(int dockDirection READ dockDirection WRITE setDockDirection NOTIFY dockDirectionChanged)
    Q_PROPERTY(int fontPointSize READ fontPointSize WRITE setFontPointSize NOTIFY fontPointSizeChanged)
    Q_PROPERTY(bool dimsWallpaper READ dimsWallpaper WRITE setDimsWallpaper NOTIFY dimsWallpaperChanged)

public:
    explicit Appearance(QObject *parent = nullptr);

    Q_INVOKABLE void switchDarkMode(bool darkMode);

    bool dimsWallpaper() const;
    Q_INVOKABLE void setDimsWallpaper(bool value);

    int dockIconSize() const;
    Q_INVOKABLE void setDockIconSize(int dockIconSize);

    int dockDirection() const;
    Q_INVOKABLE void setDockDirection(int dockDirection);

    Q_INVOKABLE void setGenericFontFamily(const QString &name);
    Q_INVOKABLE void setFixedFontFamily(const QString &name);

    int fontPointSize() const;
    Q_INVOKABLE void setFontPointSize(int fontPointSize);

    Q_INVOKABLE void setAccentColor(int accentColor);

signals:
    void dockIconSizeChanged();
    void dockDirectionChanged();
    void fontPointSizeChanged();
    void dimsWallpaperChanged();

private:
    QDBusInterface m_interface;
    DockSettings *m_dockSettings;
    int m_fontPointSize;
};

#endif // APPEARANCE_H
