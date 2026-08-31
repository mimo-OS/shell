#ifndef WINDOWINTERFACE_H
#define WINDOWINTERFACE_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>

class WindowInterface : public QObject
{
    Q_OBJECT

public:
    static WindowInterface *instance();
    explicit WindowInterface(QObject *parent = nullptr);

    quint64 activeWindow() const;
    void minimizeWindow(quint64 window);
    void closeWindow(quint64 window);
    void forceActiveWindow(quint64 window);

    QMap<QString, QVariant> requestInfo(quint64 wid) const;
    QString requestWindowClass(quint64 wid) const;
    bool isAcceptableWindow(quint64 wid) const;

    void startInitWindows();
    QString desktopFilePath(quint64 wid) const;

signals:
    void windowAdded(quint64 wid);
    void windowRemoved(quint64 wid);
    void windowChanged(quint64 wid);
    void activeChanged(quint64 wid);

private:
    void onWindowAdded(quint64 wid);
    void onWindowChanged(quint64 wid);
};

#endif // WINDOWINTERFACE_H
