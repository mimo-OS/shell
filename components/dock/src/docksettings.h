#ifndef DOCKSETTINGS_H
#define DOCKSETTINGS_H

#include <QObject>
#include <QSettings>

class DockSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(int iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)
    Q_PROPERTY(int edgeMargins READ edgeMargins WRITE setEdgeMargins)
    Q_PROPERTY(bool roundedWindowEnabled READ roundedWindowEnabled WRITE setRoundedWindowEnabled NOTIFY roundedWindowEnabledChanged)

public:
    enum Direction {
        Left = 0,
        Bottom,
        Right
    };
    Q_ENUMS(Direction)

    enum Visibility {
        AlwaysShow = 0,
        // AutoHide,
        AlwaysHide,
        IntellHide
    };
    Q_ENUMS(Visibility)

    static DockSettings *self();
    explicit DockSettings(QObject *parent = nullptr);

    int iconSize() const;
    void setIconSize(int iconSize);

    Direction direction() const;
    void setDirection(const Direction &direction);

    Visibility visibility() const;
    void setVisibility(const Visibility &visibility);

    int edgeMargins() const;
    void setEdgeMargins(int edgeMargins);

    bool roundedWindowEnabled() const;
    void setRoundedWindowEnabled(bool enabled);

signals:
    void iconSizeChanged();
    void directionChanged();
    void visibilityChanged();
    void roundedWindowEnabledChanged();

private:
    int m_iconSize;
    int m_edgeMargins;
    bool m_roundedWindowEnabled;
    Direction m_direction;
    Visibility m_visibility;
    QSettings *m_settings;
};

#endif // DOCKSETTINGS_H
