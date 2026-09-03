#ifndef DBUSMENUSHORTCUT_H
#define DBUSMENUSHORTCUT_H

// Qt
#include <QMetaType>
#include <QStringList>

class QKeySequence;

class DBusMenuShortcut : public QList<QStringList>
{
public:
    QKeySequence toKeySequence() const;
    static DBusMenuShortcut fromKeySequence(const QKeySequence &);
};

Q_DECLARE_METATYPE(DBusMenuShortcut)

#endif /* DBUSMENUSHORTCUT_H */
