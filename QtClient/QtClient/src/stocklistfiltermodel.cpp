#include "include/stocklistfiltermodel.h"

/** 
 * @brief Method to set the filter text for the filter model.
 * @param QString: new filter text.
 */
void StocklistFilterModel::setFilterText(QString ft)
{
    if (m_filterText != ft)
        m_filterText = ft;

    invalidateFilter();
}

/** 
 * @brief Apply filter to the orginal model.
 * @param int source_row.
 * @param const QModelIndex& source_parent.
 * @return bool return true if contains filter text, otherwise return false.
 */
bool StocklistFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex cur = sourceModel()->index(source_row, 0, source_parent);

    QString temp = sourceModel()->data(cur).toString();

    if (sourceModel()->data(cur).toString().contains(m_filterText, Qt::CaseInsensitive))
        return true;
    else
        return false;
}