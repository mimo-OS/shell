#ifndef VERTICALMENU_H
#define VERTICALMENU_H

#include <QDBusObjectPath>
#include <QMenu>

class VerticalMenu : public QMenu
{
    Q_OBJECT
public:
    explicit VerticalMenu(QWidget *parent = nullptr);
    ~VerticalMenu() override;

    QString serviceName() const
    {
        return m_serviceName;
    }
    void setServiceName(const QString &serviceName)
    {
        m_serviceName = serviceName;
    }

    QDBusObjectPath menuObjectPath() const
    {
        return m_menuObjectPath;
    }
    void setMenuObjectPath(const QDBusObjectPath &menuObjectPath)
    {
        m_menuObjectPath = menuObjectPath;
    }

protected:
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void paintEvent(QPaintEvent *) override;

private:
    QMenu *leafMenu();

    QString m_serviceName;
    QDBusObjectPath m_menuObjectPath;
};

#endif // VERTICALMENU_H
