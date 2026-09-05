#ifndef LAUNCHERMODEL_H
#define LAUNCHERMODEL_H

#include <QObject>
#include <QLoggingCategory>
#include <QAbstractListModel>
#include <QSettings>
#include <QTimer>

#include "appitem.h"

class LauncherModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        AppIdRole = Qt::UserRole + 1,
        ApplicationRole,
        NameRole,
        GenericNameRole,
        CommentRole,
        IconNameRole,
        CategoriesRole,
        FilterInfoRole,
        PinnedRole,
        PinnedIndexRole,
        NewInstalledRole
    };
    Q_ENUM(Roles)

    enum Mode {
        NormalMode,
        SearchMode
    };
    Q_ENUM(Mode)

    explicit LauncherModel(QObject *parent = nullptr);
    ~LauncherModel();

    int count() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE void search(const QString &key);
    Q_INVOKABLE void sendToDesktop(const QString &key);

    Q_INVOKABLE int findById(const QString &id);

    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void save();

    void delaySave();

public Q_SLOTS:
    Q_INVOKABLE bool launch(const QString &path);
    Q_INVOKABLE bool launch() { return launch(QString()); }

Q_SIGNALS:
    void countChanged();
    void refreshed();
    void applicationLaunched();

private Q_SLOTS:
    void onApplicationsChanged();

private:
    QList<AppItem> m_appItems;
    QList<AppItem> m_searchItems;

    QTimer m_saveTimer;
    QSettings m_settings;
    Mode m_mode;
    QString m_searchKey;

    bool m_firstLoad;
};

#endif // LAUNCHERMODEL_H
