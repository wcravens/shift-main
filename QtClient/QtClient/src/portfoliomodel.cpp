#include "include/portfoliomodel.h"

#include "include/qtcoreclient.h"

#include <QMutexLocker>


PortfolioModel::PortfolioModel()
{
//    m_total_realized_PL = 0.0f;

    connect(&m_timer, &QTimer::timeout, this, &PortfolioModel::refreshPortfolioPL);
    m_timer.setInterval(500); // millisecond
    m_timer.start();
}

/**
 * @brief Get how many rows should the model has.
 * @return int: number of rows for the PortfolioModel.
 */
int PortfolioModel::rowCount(const QModelIndex& parent) const
{
    return m_portfolio_item_vec.size();
}

/**
 * @brief Get how many columns should the model has.
 * @return int: number of columns for the PortfolioModel.
 */
int PortfolioModel::columnCount(const QModelIndex& parent) const
{
    return 5;
}

/**
 * @brief Method to modify what to show in each column; text alignment
 *        for each column.
 */
QVariant PortfolioModel::data(const QModelIndex& index, int role) const
{
    shift::PortfolioItem item = Global::qt_core_client.getPortfolioItems()[m_portfolio_item_vec[index.row()]];
    double price = item.getPrice();
    double shares = item.getShares();

    // calculation
    double currentPrice = Global::qt_core_client.getLastPrice(m_portfolio_item_vec[index.row()]);
    double unrealizedPL = (currentPrice - price) * shares;

    double pl = item.getPL() + unrealizedPL;
    double closePrice = (shares == 0.0 ? 0.0 : (pl / shares)) + price;

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return QString::fromStdString(m_portfolio_item_vec[index.row()]);
            break;

        case 1:
            return shares;
            break;

        case 2:
            return QString::number(price, 'f', 2);
            break;

        case 3:
            return QString::number(closePrice, 'f', 2);
            break;

        case 4:
            return QString::number(pl, 'f', 2);
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
    }
    return QVariant::Invalid;
}

/**
 * @brief Method to set the header for the QTableView.
 */
QVariant PortfolioModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case 0:
                return QString("Symbol");
                break;
            case 1:
                return QString("Current Shares");
                break;
            case 2:
                return QString("Traded Price");
                break;
            case 3:
                return QString("Close Price");
                break;
            case 4:
                return QString("P&L");
                break;
            default:
                break;
            }
        }
    }
    return QVariant::Invalid;
}

void PortfolioModel::updatePortfolioItem(std::string symbol)
{
    // find symbol
    int index = -1;
    for (int i = 0; i < m_portfolio_item_vec.size(); i++) {
        if (m_portfolio_item_vec[i] == symbol) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        if (symbol != "") {
            // insert portfolio
            beginInsertRows(QModelIndex(), m_portfolio_item_vec.size(), m_portfolio_item_vec.size());
            m_portfolio_item_vec.push_back(symbol);
            endInsertRows();
        }
    } else {
        emit dataChanged(this->index(index, 1), this->index(index, 2));
    }

}

void PortfolioModel::updatePortfolioSummary()
{
    m_portfolio_summary = Global::qt_core_client.getPortfolioSummary();
    emit updateTotalPortfolio(m_portfolio_summary.getTotalBP(), m_portfolio_summary.getTotalShares());
    refreshPortfolioPL();
}

/** 
 * @brief Method to refresh buying power/traded shares/totalPL.
 */
void PortfolioModel::refreshPortfolioPL()
{
//    QMutexLocker lock(&m_mutex);
    double total_PL = m_portfolio_summary.getTotalPL();

//    if (!m_portfolio_item.empty()) {
//        for (int i = 0; i < m_portfolio_item.size(); i++) {
////            std::string symbol = m_portfolio_item[i].m_symbol.toStdString();
//            double current_price = QtCoreClient::getInstance() -> getLatestPrice(m_portfolio_item[i]);
//            double unrealized_pl = (current_price - QtCoreClient::getInstance() -> getPortfolioItems()[m_portfolio_item[i]].getPrice()) * QtCoreClient::getInstance() -> getPortfolioItems()[m_portfolio_item[i]].getShares();
//            total_PL += unrealized_pl;
//            double current_pl = unrealized_pl +  QtCoreClient::getInstance() -> getPortfolioItems()[m_portfolio_item[i]].getPL();
//            QString pl_str = QString::number(current_pl, 'f', 2);
//            m_portfolio_item[i].m_pl = pl_str;
////            if (m_portfolio_item[i].m_shares != 0)
////                m_portfolio_item[i].m_close_price = QString::number(current_pl / m_portfolio_item[i].m_shares + m_portfolio_item[i].m_price.toDouble(), 'f', 2);
////            else
////                m_portfolio_item[i].m_close_price = "0.00";
//        }
//        emit dataChanged(this->index(0, 3), this->index(m_portfolio_item.size() - 1, 4));
//    }
    emit updateTotalPL(total_PL);
}
