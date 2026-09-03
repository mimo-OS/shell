#ifndef APPITEM_H
#define APPITEM_H

#include <QString>
#include <QStringList>
#include <QMetaType>

class AppItem
{
public:
    AppItem();
    AppItem(const AppItem &info);
    ~AppItem();

    inline bool operator==(const AppItem &other) const { return id == other.id; }
    friend QDataStream &operator<<(QDataStream &argument, const AppItem &info);
    friend const QDataStream &operator>>(QDataStream &argument, AppItem &info);

    QString id;
    QString name;
    QString genericName;
    QString comment;
    QString iconName;
    QStringList args;

    bool newInstalled;
};

Q_DECLARE_METATYPE(AppItem)

#endif // APPITEM_H
