#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "shellsurface.h"
#include "activity.h"

class StatusBar : public ShellSurface
{
    Q_OBJECT
    Q_PROPERTY(QRect screenRect READ screenRect NOTIFY screenRectChanged)
    Q_PROPERTY(bool twentyFourTime READ twentyFourTime NOTIFY twentyFourTimeChanged)

public:
    explicit StatusBar(QQmlEngine *engine);

    QRect screenRect();
    bool twentyFourTime();

    void setBatteryPercentage(bool enabled);
    void setTwentyFourTime(bool t);

    void updateGeometry();
    void updateViewStruts();

signals:
    void screenRectChanged();
    void launchPadChanged();
    void twentyFourTimeChanged();

private slots:
    void initState();
    void onPrimaryScreenChanged(QScreen *screen);

private:
    QRect m_screenRect;
    Activity *m_acticity;
    bool m_twentyFourTime;
};

#endif // STATUSBAR_H
