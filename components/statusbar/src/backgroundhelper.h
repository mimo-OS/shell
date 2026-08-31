#ifndef BACKGROUNDHELPER_H
#define BACKGROUNDHELPER_H

#include <QObject>
#include <QColor>
#include <QPointer>

class QScreen;

class BackgroundHelper : public QObject
{
    Q_OBJECT

public:
    explicit BackgroundHelper(QObject *parent = nullptr);

    Q_INVOKABLE void setColor(QColor c);
    Q_INVOKABLE void setBackgound(const QString &fileName);

private slots:
    void onPrimaryScreenChanged();
    void onChanged();

signals:
    void newColor(QColor color, bool darkMode);

private:
    QPointer<QScreen> m_screen;
    int m_statusBarHeight;
    int m_type;
    QColor m_color;
    QString m_wallpaper;
};

#endif // BACKGROUNDHELPER_H
