#pragma once

#include "global.h"
#include "include/orderbookmodel.h"
#include "include/qtcoreclient.h"
#include "include/stocklistfiltermodel.h"

#include <QDialog>
#include <QStringListModel>
#include <QTimer>

namespace Ui {
class OrderBookDialog;
}

/**
 * @brief Class which defined the OrderBook dialog of the application, including .cpp and .ui,
 *        extends QDialog.
 */
class OrderBookDialog : public QDialog {
    Q_OBJECT

public:
    explicit OrderBookDialog(QWidget* parent = nullptr);
    ~OrderBookDialog();
    void resume();

public slots:
    void refreshData();
    void onStockListIndexChanged(const QModelIndex& index);
    void setStocklist(QStringList stocklist);
    void receiveOpenPrice(QString symbol, double openPrice);

private:
    Ui::OrderBookDialog* ui;
    static OrderBookDialog* m_instance;
    QTimer* m_timer;

    OrderBookModel m_global_bid_data;
    OrderBookModel m_global_ask_data;
    OrderBookModel m_local_bid_data;
    OrderBookModel m_local_ask_data;

    QString m_current_symbol; //!< The symbol whose chart is showing.
    QStringListModel* m_stock_list_model; //!< Model to be used in QTableView stocklist.
    StocklistFilterModel* m_filter_model; //!< Filter Model used to implement progressive search feature.
    int m_last_clicked_index = 0; //!< The index of the last clicked stock.

    QMap<QString, double> m_open_price_list;
};
