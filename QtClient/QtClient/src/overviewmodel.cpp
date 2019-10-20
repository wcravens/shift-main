#include "include/overviewmodel.h"

#include "include/qtcoreclient.h"

#include <QColor>
#include <QFont>

OverviewModel::OverviewModel()
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &OverviewModel::refresh);
}

/**
 * @brief Get how many rows should the model has.
 * @return int: number of rows for the OverviewModel.
 */
int OverviewModel::rowCount(const QModelIndex& parent) const
{
    return m_new_overview.size();
}

/**
 * @brief Get how many columns should the model has.
 * @return int: number of columns for the OverviewModel.
 */
int OverviewModel::columnCount(const QModelIndex& parent) const
{
    return 6;
}

/**
 * @brief Method to modify what to show in each column; text alignment
 *        for each column.
 */
QVariant OverviewModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return m_new_overview[index.row()].m_symbol;
            break;
        case 1:
            return m_new_overview[index.row()].m_last_price.first;
            break;
        case 2:
            return m_new_overview[index.row()].m_bid_size;
            break;
        case 3:
            return m_new_overview[index.row()].m_bid_price.first;
            break;
        case 4:
            return m_new_overview[index.row()].m_ask_price.first;
            break;
        case 5:
            return m_new_overview[index.row()].m_ask_size;
            break;
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0) {
            return Qt::AlignCenter;
        } else {
            return Qt::AlignRight + Qt::AlignVCenter;
        }
    } else if (role == Qt::ForegroundRole) { // setup text color change
        if (index.column() == 1) {
            if (m_new_overview[index.row()].m_last_price.second == 0)
                return QVariant(QColor(162, 6, 6));
            else if (m_new_overview[index.row()].m_last_price.second == 1)
                return QVariant(QColor(6, 162, 6));
            else if (m_new_overview[index.row()].m_last_price.second == 2)
                return QVariant(QColor(Qt::white));
        } else if (index.column() == 3) {
            if (m_new_overview[index.row()].m_bid_price.second == 0)
                return QVariant(QColor(162, 6, 6));
            else if (m_new_overview[index.row()].m_bid_price.second == 1)
                return QVariant(QColor(6, 162, 6));
            else if (m_new_overview[index.row()].m_bid_price.second == 2)
                return QVariant(QColor(Qt::white));
        } else if (index.column() == 4) {
            if (m_new_overview[index.row()].m_ask_price.second == 0)
                return QVariant(QColor(162, 6, 6));
            else if (m_new_overview[index.row()].m_ask_price.second == 1)
                return QVariant(QColor(6, 162, 6));
            else if (m_new_overview[index.row()].m_ask_price.second == 2)
                return QVariant(QColor(Qt::white));
        }
    } else if (role == Qt::FontRole) { // setup bold text
        QFont boldFont;
        boldFont.setBold(true);

        QFont regularFont;
        if (index.column() == 1) {
            if (m_new_overview[index.row()].m_last_price.second == 0)
                return boldFont;
            else if (m_new_overview[index.row()].m_last_price.second == 1)
                return boldFont;
            else if (m_new_overview[index.row()].m_last_price.second == 2)
                return regularFont;
        } else if (index.column() == 3) {
            if (m_new_overview[index.row()].m_bid_price.second == 0)
                return boldFont;
            else if (m_new_overview[index.row()].m_bid_price.second == 1)
                return boldFont;
            else if (m_new_overview[index.row()].m_bid_price.second == 2)
                return regularFont;
        } else if (index.column() == 4) {
            if (m_new_overview[index.row()].m_ask_price.second == 0)
                return boldFont;
            else if (m_new_overview[index.row()].m_ask_price.second == 1)
                return boldFont;
            else if (m_new_overview[index.row()].m_ask_price.second == 2)
                return regularFont;
        } else if (index.column() == 0) {
            return boldFont;
        }
    }

    return QVariant::Invalid;
}

/**
 * @brief Method to set the header for the QTableView.
 */
QVariant OverviewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case 0:
                return QString("Symbol");
            case 1:
                return QString("Last Price");
            case 2:
                return QString("Bid Size");
            case 3:
                return QString("Bid Price");
            case 4:
                return QString("Ask Price");
            case 5:
                return QString("Ask Size");
            default:
                break;
            }
        }
    } else if (role == Qt::FontRole) {
        QFont boldFont;
        boldFont.setBold(true);
        return boldFont;
    }

    return QVariant::Invalid;
}

/**
 * @brief Find the index of the target symbol name.
 * @param QString: name of the target symbol.
 * @return int: index of the symbol, or -1 if not found.
 */
int OverviewModel::findRowIndex(QString symbol)
{
    // return -1 if cannot find the symbol
    for (int i = 0; i < m_new_overview.size(); ++i) {
        if (m_new_overview[i].m_symbol == symbol) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Method functions as when all stocklist are received, start the refresh timer.
 */
void OverviewModel::receiveStocklistReady()
{
    QStringList stocklist = Global::qt_core_client.getStocklist();
    beginInsertRows(QModelIndex(), 0, stocklist.size() - 1);
    for (int i = 0; i < stocklist.size(); ++i) {
        OverviewModelItem item(stocklist[i], QPair<QString, int>("0", 2), 0, QPair<QString, int>("0", 2), QPair<QString, int>("0", 2), 0);
        m_new_overview.push_back(item);
    }
    endInsertRows();

    // start timer
    m_timer.start();
}

/**
 * @brief Method to refresh the data in overview table.
 */
void OverviewModel::refresh()
{
    QStringList stocklist = Global::qt_core_client.getStocklist();

    resetInternalData();
    beginInsertRows(QModelIndex(), 0, stocklist.size() - 1);
    for (int i = 0; i < stocklist.size(); ++i) {
        QString symbol = stocklist[i];

        double last = QString::number(Global::qt_core_client.getLastPrice(symbol.toStdString()), 'f', 2).toDouble();
        double lastDiff = last - m_prev_overview[i].m_last_price.first.toDouble();
        int lastChange;
        if (lastDiff > 0)
            lastChange = 1;
        else if (lastDiff < 0)
            lastChange = 0;
        else
            lastChange = 2;

        int bidSize = Global::qt_core_client.getBestPrice(symbol.toStdString()).getBidSize();

        double bid = QString::number(Global::qt_core_client.getBestPrice(symbol.toStdString()).getBidPrice(), 'f', 2).toDouble();
        double bidDiff = bid - m_prev_overview[i].m_bid_price.first.toDouble();
        int bidChange;
        if (bidDiff > 0)
            bidChange = 1;
        else if (bidDiff < 0)
            bidChange = 0;
        else
            bidChange = 2;

        int askSize = Global::qt_core_client.getBestPrice(symbol.toStdString()).getAskSize();

        double ask = QString::number(Global::qt_core_client.getBestPrice(symbol.toStdString()).getAskPrice(), 'f', 2).toDouble();
        double askDiff = ask - m_prev_overview[i].m_ask_price.first.toDouble();
        int askChange;
        if (askDiff > 0)
            askChange = 1;
        else if (askDiff < 0)
            askChange = 0;
        else
            askChange = 2;

        OverviewModelItem item(symbol, QPair<QString, int>(QString::number(last, 'f', 2), lastChange), bidSize, QPair<QString, int>(QString::number(bid, 'f', 2), bidChange), QPair<QString, int>(QString::number(ask, 'f', 2), askChange), askSize);
        m_new_overview.push_back(item);
    }

    endInsertRows();
}

/**
 * @brief Remove everything and reset the model.
 */
void OverviewModel::resetInternalData()
{
    if (!m_new_overview.empty()) {
        beginRemoveRows(QModelIndex(), 0, m_new_overview.size() - 1);
        m_prev_overview = m_new_overview;
        m_new_overview.clear();
        endRemoveRows();
    }
}

/**
 * @brief When the table is clicked. Emit setSendOrder signal to set the textedit.
 */
void OverviewModel::onClicked(const QModelIndex& index)
{
    int row = index.row();
    if (row < m_new_overview.size()) {
        QString symbol = m_new_overview[row].m_symbol;
        double price = m_new_overview[row].m_last_price.first.toDouble();
        emit setSendOrder(symbol, price);
    }
}
