#include "pagemodel.h"

#include <QtCore/QtGlobal>

PageModel::PageModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool PageModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);

    return sourceRow >= m_startIndex && sourceRow < m_startIndex + m_limitCount;
}

int PageModel::startIndex() const { return m_startIndex; }

int PageModel::limitCount() const { return m_limitCount; }

void PageModel::move(int from, int to)
{
    if (from == to)
        return;

    if (from < to)
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to + 1);
    else
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);

    endMoveRows();
}

void PageModel::setStartIndex(int startIndex)
{
    if (startIndex != m_startIndex) {
        m_startIndex = startIndex;
        invalidateFilter();
        Q_EMIT startIndexChanged();
    }
}

void PageModel::setLimitCount(int limitCount)
{
    if (limitCount != m_limitCount) {
        m_limitCount = limitCount;
        invalidateFilter();
        Q_EMIT limitCountChanged();
    }
}
