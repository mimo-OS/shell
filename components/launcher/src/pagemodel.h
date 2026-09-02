#ifndef PAGEMODEL_H
#define PAGEMODEL_H

#include <QtCore/QAbstractProxyModel>

/**
 * Exposes rows [startIndex, startIndex + limitCount) of a source model as one
 * page of the launcher grid.
 *
 * Not a QSortFilterProxyModel: that reports a source row move as
 * layoutChanged(), which makes QQmlDelegateModel rebuild every delegate and so
 * destroys the one a drag is running on.
 */
class PageModel : public QAbstractProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int startIndex READ startIndex WRITE setStartIndex NOTIFY startIndexChanged)
    Q_PROPERTY(int limitCount READ limitCount WRITE setLimitCount NOTIFY limitCountChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    PageModel(QObject *parent = nullptr);

    int startIndex() const;
    int limitCount() const;
    int count() const;

    void setSourceModel(QAbstractItemModel *model) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

public Q_SLOTS:
    void setStartIndex(int startIndex);
    void setLimitCount(int limitCount);

Q_SIGNALS:
    void startIndexChanged();
    void limitCountChanged();
    void countChanged();

private:
    void connectSource(QAbstractItemModel *model);
    void rebuild();

    void onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                             const QList<int> &roles);
    void onSourceRowsAboutToBeMoved(const QModelIndex &parent, int start, int end,
                                    const QModelIndex &destination, int row);
    void onSourceRowsMoved();

    int m_startIndex = 0;
    int m_limitCount = 0;
    int m_count = 0;

    enum class Pending { None, Move, Reset };
    Pending m_pending = Pending::None;
};

#endif // PAGEMODEL_H
