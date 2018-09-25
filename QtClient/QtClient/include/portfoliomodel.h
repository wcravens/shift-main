#pragma once

#include <QAbstractTableModel>
#include <QMutex>
#include <QTimer>
#include <QVariant>
#include <QVector>

#include <PortfolioSummary.h>
#include "global.h"

///**
// * @brief struct of object use as items for the PortfolioModel.
// */
//struct PortfolioModelItem {
//    QString m_symbol;
//    int m_shares;
//    QString m_price;
//    QString m_close_price;
//    QString m_pl;
//    double m_realized_PL;

//    /**
//     * @brief Default constructor for PortfolioModelItem.
//     */
//    PortfolioModelItem()
//        : m_symbol("")
//        , m_shares(0)
//        , m_price("0.00")
//        , m_close_price("0.00")
//        , m_pl("0.00")
//        , m_realized_PL(0.0)
//    {
//    }

//    /**
//     * @brief Constructor for PortfolioModelItem with all members preset.
//     */
//    PortfolioModelItem(QString symbol, int shares, QString price, QString close_price, QString pl, double realized_PL)
//        : m_symbol(symbol)
//        , m_shares(shares)
//        , m_price(price)
//        , m_close_price(close_price)
//        , m_pl(pl)
//        , m_realized_PL(realized_PL)
//    {
//    }
//};

/**
 * @brief Class of PortfolioModel. Extends from QAbstractTableModel. To be used in
 *        QTableView.
 */
class PortfolioModel : public QAbstractTableModel {
    Q_OBJECT

public:
    PortfolioModel();

    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

signals:
    void updateTotalPortfolio(double total_BP, int total_shares);
    void updateTotalPL(double total_PL);

public slots:
    void receivePortfolio(std::string symbol);
    void refreshPortfolioPL();

protected:
    QMutex m_mutex;
    QVector<std::string> m_portfolio_item_vec;
//    double m_total_realized_PL;
    shift::PortfolioSummary m_portfolio_summary;
    QTimer m_timer;
};
