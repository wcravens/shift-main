#include "include/orderbookmodel.h"

#include <QModelIndex>

OrderBookModel::OrderBookModel()
{
}

int OrderBookModel::rowCount(const QModelIndex&) const
{
    return m_order_books.size();
}

int OrderBookModel::columnCount(const QModelIndex&) const
{
    return 2;
}

/**
 * @brief Method to modify what to show in each column; text alignment
 *        for each column.
 */
QVariant OrderBookModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return m_order_books[index.row()].m_price;
            break;
        case 1:
            return m_order_books[index.row()].m_size;
            break;
        //        case 2:
        //            return m_OrderBooks[index.row()].destination;
        //            break;
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 2)
            return Qt::AlignCenter;
        else
            return Qt::AlignRight + Qt::AlignVCenter;
    }
    return QVariant::Invalid;
}

/**
 * @brief Method to set the header for the QTableView.
 */
QVariant OrderBookModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case 0:
                return QString("Price");
                break;
            case 1:
                return QString("Size");
                break;
            //            case 2:
            //                return QString("Destination");
            //                break;
            default:
                break;
            }
        }
    }
    return QVariant::Invalid;
}

/** 
 * @brief Method to remove several rows from the Model.
 * @param int: starting index for the deletion.
 * @param int: number of rows to be deleted.
 * @param index: index to the TableView.
 * @return bool: true.
 */
bool OrderBookModel::removeRows(int position, int rows, const QModelIndex& index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int i = 0; i < rows; ++i) {
        m_order_books.removeAt(position);
    }

    endRemoveRows();
    return true;
}

/**
 * @brief Method to update current orderbook model with new order book entries.
 * @param std::vector<shift::OrderBookEntry>: List of orders to be updated into the model.
 */
void OrderBookModel::updateData(std::vector<shift::OrderBookEntry> entries)
{
    resetInternalData();
    if (!entries.empty()) {
        beginInsertRows(QModelIndex(), 0, entries.size() - 1);
        for (int i = 0; i < entries.size(); ++i) {
            OrderBookItem item = OrderBookItem(QString::number(entries[i].getPrice(), 'f', 2), entries[i].getSize(), QString::fromStdString(entries[i].getDestination()));
            m_order_books.push_back(item);
        }
        endInsertRows();
    }
}

/**
 * @brief Remove everything and reset the model.
 */
void OrderBookModel::resetInternalData()
{
    if (!m_order_books.empty()) {
        beginRemoveRows(QModelIndex(), 0, m_order_books.size() - 1);
        m_order_books.clear();
        endRemoveRows();
    }
}
