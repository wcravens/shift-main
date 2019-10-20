#include "include/portfoliomodel.h"

#include "include/qtcoreclient.h"

#include <QDebug>
#include <QMutexLocker>

PortfolioModel::PortfolioModel()
{
    QTimer* t = new QTimer(this);
    t->setInterval(1000);
    t->setSingleShot(false);
    connect(t, &QTimer::timeout, [this]() {
        for (std::string symbol : m_portfolio_item_vec)
            updatePortfolioItem(symbol);
    });
    t->start();
}

/**
 * @brief Get how many rows should the model has.
 * @return int: number of rows for the PortfolioModel.
 */
int PortfolioModel::rowCount(const QModelIndex&) const
{
    return m_portfolio_item_vec.size();
}

/**
 * @brief Get how many columns should the model has.
 * @return int: number of columns for the PortfolioModel.
 */
int PortfolioModel::columnCount(const QModelIndex&) const
{
    return 6;
}

/**
 * @brief Method to modify what to show in each column; text alignment
 *        for each column.
 */
QVariant PortfolioModel::data(const QModelIndex& index, int role) const
{
    shift::PortfolioItem item = Global::qt_core_client.getPortfolioItems()[m_portfolio_item_vec[index.row()]];
    double tradedPrice = item.getPrice();
    int currentShares = item.getShares();

    // calculation
    bool buy = (currentShares < 0);
    double closePrice = (currentShares == 0) ? 0.0 : Global::qt_core_client.getClosePrice(item.getSymbol(), buy, currentShares / 100);
    double unrealizedPL = (closePrice - tradedPrice) * currentShares;
    double pl = item.getRealizedPL() + unrealizedPL;

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return QString::fromStdString(m_portfolio_item_vec[index.row()]);
            break;

        case 1:
            return currentShares;
            break;

        case 2:
            return QString::number(tradedPrice, 'f', 2);
            break;

        case 3:
            return QString::number(closePrice, 'f', 2);
            break;

        case 4:
            return QString::number(unrealizedPL, 'f', 2);
            break;

        case 5:
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
                return QString("Unrealized P&L");
                break;
            case 5:
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
    for (int i = 0; i < m_portfolio_item_vec.size(); ++i) {
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
    emit updateTotalPortfolio(m_portfolio_summary.getTotalBP(), m_portfolio_summary.getTotalShares(), m_portfolio_summary.getTotalRealizedPL());
}
