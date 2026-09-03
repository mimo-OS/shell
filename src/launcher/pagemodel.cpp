#include "pagemodel.h"

#include <QtCore/QtGlobal>

PageModel::PageModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

int PageModel::startIndex() const { return m_startIndex; }

int PageModel::limitCount() const { return m_limitCount; }

int PageModel::count() const { return m_count; }

void PageModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel() == model)
        return;

    if (sourceModel())
        sourceModel()->disconnect(this);

    beginResetModel();
    QAbstractProxyModel::setSourceModel(model);
    connectSource(model);
    m_count = rowCount();
    endResetModel();
    emit countChanged();
}

void PageModel::connectSource(QAbstractItemModel *model)
{
    if (!model)
        return;

    // An insert or removal shifts every page after it; nothing finer to report.
    connect(model, &QAbstractItemModel::modelReset, this, &PageModel::rebuild);
    connect(model, &QAbstractItemModel::layoutChanged, this, &PageModel::rebuild);
    connect(model, &QAbstractItemModel::rowsInserted, this, &PageModel::rebuild);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &PageModel::rebuild);

    connect(model, &QAbstractItemModel::dataChanged, this, &PageModel::onSourceDataChanged);
    connect(model, &QAbstractItemModel::rowsAboutToBeMoved,
            this, &PageModel::onSourceRowsAboutToBeMoved);
    connect(model, &QAbstractItemModel::rowsMoved, this, &PageModel::onSourceRowsMoved);
}

void PageModel::rebuild()
{
    beginResetModel();
    const int previous = m_count;
    m_count = rowCount();
    endResetModel();

    if (previous != m_count)
        emit countChanged();
}

int PageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !sourceModel())
        return 0;

    const int available = sourceModel()->rowCount() - m_startIndex;
    return qBound(0, available, m_limitCount);
}

int PageModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !sourceModel())
        return 0;
    return sourceModel()->columnCount();
}

QModelIndex PageModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
        return {};
    return createIndex(row, column);
}

QModelIndex PageModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return {};
}

QModelIndex PageModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid() || !sourceModel())
        return {};
    return sourceModel()->index(proxyIndex.row() + m_startIndex, proxyIndex.column());
}

QModelIndex PageModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.parent().isValid())
        return {};

    const int row = sourceIndex.row() - m_startIndex;
    if (row < 0 || row >= rowCount())
        return {};

    return createIndex(row, sourceIndex.column());
}

void PageModel::onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                    const QList<int> &roles)
{
    const int first = qMax(topLeft.row(), m_startIndex);
    const int last = qMin(bottomRight.row(), m_startIndex + rowCount() - 1);
    if (first > last)
        return;

    emit dataChanged(index(first - m_startIndex, topLeft.column()),
                     index(last - m_startIndex, bottomRight.column()), roles);
}

void PageModel::onSourceRowsAboutToBeMoved(const QModelIndex &parent, int start, int end,
                                           const QModelIndex &destination, int row)
{
    const int rows = rowCount();
    const int proxyStart = start - m_startIndex;
    const int proxyEnd = end - m_startIndex;
    const int proxyRow = row - m_startIndex;

    // Only a move that stays inside the page is still a move here.
    const bool insidePage = !parent.isValid() && !destination.isValid()
            && proxyStart >= 0 && proxyEnd < rows
            && proxyRow >= 0 && proxyRow <= rows;

    if (insidePage && beginMoveRows({}, proxyStart, proxyEnd, {}, proxyRow)) {
        m_pending = Pending::Move;
        return;
    }

    m_pending = Pending::Reset;
    beginResetModel();
}

void PageModel::onSourceRowsMoved()
{
    switch (m_pending) {
    case Pending::Move:
        endMoveRows();
        break;
    case Pending::Reset: {
        const int previous = m_count;
        m_count = rowCount();
        endResetModel();
        if (previous != m_count)
            emit countChanged();
        break;
    }
    case Pending::None:
        break;
    }

    m_pending = Pending::None;
}

void PageModel::setStartIndex(int startIndex)
{
    if (startIndex != m_startIndex) {
        beginResetModel();
        m_startIndex = startIndex;
        m_count = rowCount();
        endResetModel();
        emit startIndexChanged();
        emit countChanged();
    }
}

void PageModel::setLimitCount(int limitCount)
{
    if (limitCount != m_limitCount) {
        beginResetModel();
        m_limitCount = limitCount;
        m_count = rowCount();
        endResetModel();
        emit limitCountChanged();
        emit countChanged();
    }
}
