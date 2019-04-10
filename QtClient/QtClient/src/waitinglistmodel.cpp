#include "include/waitinglistmodel.h"

#include <QModelIndex>


WaitingListModel::WaitingListModel()
{
}

int WaitingListModel::rowCount(const QModelIndex&) const
{
    return m_waiting_list_item.size();
}

int WaitingListModel::columnCount(const QModelIndex&) const
{
    return 5;
}

/**
 * @brief Method to modify what to show in each column; text alignment
 *        for each column.
 */
QVariant WaitingListModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return m_waiting_list_item[index.row()].m_symbol;
        case 1:
            return m_waiting_list_item[index.row()].m_price;
        case 2:
            return m_waiting_list_item[index.row()].m_size;
        case 3:
            return m_waiting_list_item[index.row()].m_type == '1' ? "Limit Buy" : "Limit Sell";
        case 4:
            return m_waiting_list_item[index.row()].m_id;
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0) {
            return Qt::AlignCenter;
        } else {
            return Qt::AlignRight + Qt::AlignVCenter;
        }
    }
    return QVariant::Invalid;
}

/**
 * @brief Method to set the header for the QTableView.
 */
QVariant WaitingListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case 0:
                return QString("Symbol");
            case 1:
                return QString("Price");
            case 2:
                return QString("Size");
            case 3:
                return QString("Type");
            case 4:
                return QString("Order ID");
            default:
                break;
            }
        }
    }
    return QVariant::Invalid;
}

bool WaitingListModel::removeRows(const int &begin, const int &rows, const QModelIndex& index)
{
    if (rows < 1)
        return false;
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), begin, begin + rows - 1);

    for (int i = 0; i < rows; i++) {
        m_waiting_list_item.removeAt(begin);
    }

    endRemoveRows();
    return true;
}

/**
 * @brief Update current m_waitinglist_item with newly received waitinglist.
 * @param QVector<shift::Order>: the new waitinglist to be updated into m_waitinglist_item.
 */
void WaitingListModel::receiveWaitingList(QVector<shift::Order> waiting_list)
{
    resetInternalData();
    if (waiting_list.empty())
        return;
    beginInsertRows(QModelIndex(), 0, waiting_list.size() - 1);
    for (shift::Order order : waiting_list) {
        WaitingListModelItem item(QString::fromStdString(order.getID()), QString::fromStdString(order.getSymbol()), QString::number(order.getPrice()), order.getSize(), order.getType());
        m_waiting_list_item.push_back(item);
    }
    endInsertRows();
}

/**
 * @brief Remove everything and reset the model.
 */
void WaitingListModel::resetInternalData()
{
    if (!m_waiting_list_item.empty()) {
        beginRemoveRows(QModelIndex(), 0, m_waiting_list_item.size() - 1);
        m_waiting_list_item.clear();
        endRemoveRows();
    }
}

/**
 * @brief When the table is clicked. Emit setCancelOrder signal to set the textedit.
 */
void WaitingListModel::onClicked(const QModelIndex& index)
{
    QString id, size;
    WaitingListModelItem item = m_waiting_list_item[index.row()];
    id = item.m_id;
    size = QString::number(item.m_size);
    emit setCancelOrder(id, size);
}

/** 
 * @brief Method to get all info and change OrderType by searching for the corresponding 
 *        order id from the waitinglist. Save all order info into Order reference q.
 * @param QString: target id to be searched.
 * @param shift::Order&: reference where all cancel order info are saved.
 * @return bool: true if found the order, other wise false.
 */
bool WaitingListModel::getCancelOrderByID(QString id, shift::Order& order)
{
    for (WaitingListModelItem item : m_waiting_list_item) {
        if (id == item.m_id) {
            order.setID(id.toStdString());
            order.setPrice(item.m_price.toDouble());
            order.setSize(item.m_size);
            order.setSymbol(item.m_symbol.toStdString());
            if (item.m_type == shift::Order::LIMIT_BUY) {
                order.setType(shift::Order::CANCEL_BID);
            } else if (item.m_type == shift::Order::LIMIT_SELL) {
                order.setType(shift::Order::CANCEL_ASK);
            }
            return true;
        }
    }
    return false;
}

/** 
 * @brief Get all waitinglist order with all their orderType set to be cancelled.
 * @return std::vector<shift::Order>: A vector of all orders to be cancelled.
 */
std::vector<shift::Order> WaitingListModel::getAllOrders()
{
    std::vector<shift::Order> cancel_list;
    for (WaitingListModelItem item : m_waiting_list_item) {
        shift::Order order;
        order.setID(item.m_id.toStdString());
        order.setPrice(item.m_price.toDouble());
        order.setSize(item.m_size);
        order.setSymbol(item.m_symbol.toStdString());
        if (item.m_type == shift::Order::LIMIT_BUY) {
            order.setType(shift::Order::CANCEL_BID);
        } else if (item.m_type == shift::Order::LIMIT_SELL) {
            order.setType(shift::Order::CANCEL_ASK);
        }
        cancel_list.push_back(order);
    }
    return cancel_list;
}
