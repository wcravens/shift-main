#pragma once

#include <QSortFilterProxyModel>
#include <QString>

/** 
 * @brief A FilterModel used to implement progressive search in stocklist.
 *        Extended from QSortFilterProxyModel, with self-defined filter restriction.
 */
class StocklistFilterModel : public QSortFilterProxyModel {
    Q_OBJECT

public slots:
    void setFilterText(QString ft);

private:
    QString m_filterText; //!< the text tobe filtered through the filter model.

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
};
