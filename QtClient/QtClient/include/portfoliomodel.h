#pragma once

#include <QAbstractTableModel>
#include <QMutex>
#include <QTimer>
#include <QVariant>
#include <QVector>

#include "global.h"
#include <PortfolioSummary.h>

/**
 * @brief Class of PortfolioModel. Extends from QAbstractTableModel. To be used in
 *        QTableView.
 */
class PortfolioModel : public QAbstractTableModel {
    Q_OBJECT

public:
    PortfolioModel();

    int rowCount(const QModelIndex&) const;
    int columnCount(const QModelIndex&) const;
    QVariant data(const QModelIndex& index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

signals:
    void updateTotalPortfolio(double total_BP, int total_shares, double total_PL);

public slots:
    void updatePortfolioItem(std::string symbol);
    void updatePortfolioSummary();

protected:
    QMutex m_mutex;
    QVector<std::string> m_portfolio_item_vec;
    //    double m_total_realized_PL;
    shift::PortfolioSummary m_portfolio_summary;
    QTimer m_timer;
};
