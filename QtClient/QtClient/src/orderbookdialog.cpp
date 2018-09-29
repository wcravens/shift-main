#include "include/orderbookdialog.h"
#include "ui_orderbookdialog.h"

#include <QDebug>
#include <QMessageBox>

OrderBookDialog* OrderBookDialog::m_instance = nullptr;

OrderBookDialog::OrderBookDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::OrderBookDialog())
    , m_timer(new QTimer(this))
    , m_stock_list_model(new QStringListModel(this))
    , m_filter_model(new StocklistFilterModel())
{
    this->setWindowTitle("Order Book - SHIFT Alpha");

    ui->setupUi(this);
    ui->GlobalAskTable->setModel(&m_global_ask_data);
    ui->GlobalBidTable->setModel(&m_global_bid_data);
    ui->LocalAskTable->setModel(&m_local_ask_data);
    ui->LocalBidTable->setModel(&m_local_bid_data);
    ui->StockList->setModel(m_filter_model);
    ui->LastPriceLabel->hide();
    ui->PriceDifferenceLabel->hide();
    ui->DiffPercentLabel->hide();
    ui->ArrowLabel->hide();
    ui->AcknowlageLabel->hide();
    ui->USD->hide();
    for (int i = 0; i < m_global_bid_data.columnCount(QModelIndex()); i++)
        ui->GlobalBidTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    for (int i = 0; i < m_global_ask_data.columnCount(QModelIndex()); i++)
        ui->GlobalAskTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    for (int i = 0; i < m_local_bid_data.columnCount(QModelIndex()); i++)
        ui->LocalBidTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    for (int i = 0; i < m_local_ask_data.columnCount(QModelIndex()); i++)
        ui->LocalAskTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);

    m_filter_model->setSourceModel(m_stock_list_model);
    m_timer->setInterval(500);

    connect(ui->StockList, &QTableView::clicked, this, &OrderBookDialog::onStockListIndexChanged);
    connect(ui->SearchSymbol, &QLineEdit::textChanged, m_filter_model, &StocklistFilterModel::setFilterText);
    connect(m_timer, &QTimer::timeout, this, &OrderBookDialog::refreshData);
}

void OrderBookDialog::refreshData()
{
    QString companyName = QString::fromStdString(Global::qt_core_client.getCompanyNameBySymbol(m_current_symbol.toStdString()));
    ui->TickerNameLabel->setText(m_current_symbol + (companyName == "" ? "" : " (" + companyName + ")"));

    if (m_current_symbol != "") {
        m_global_bid_data.updateData(Global::qt_core_client.getOrderBook(m_current_symbol.toStdString(), 'B'));
        m_global_ask_data.updateData(Global::qt_core_client.getOrderBook(m_current_symbol.toStdString(), 'A'));
        m_local_bid_data.updateData(Global::qt_core_client.getOrderBook(m_current_symbol.toStdString(), 'b'));
        m_local_ask_data.updateData(Global::qt_core_client.getOrderBook(m_current_symbol.toStdString(), 'a'));

        //        auto latestPriceBook = Global::qt_core_client.getLatestPriceBook();
        //        double latestPrice = latestPriceBook[real_name];

        //        if (latestPrice) {
        //            double curPrice = ui->LastPriceLabel->text().toDouble();

        //            if (latestPrice > curPrice) {
        //                ui->LastPriceLabel->setText("<font color='green'>" + QString::number(latestPrice) + "</font>");
        //            } else if (latestPrice < curPrice) {
        //                ui->LastPriceLabel->setText("<font color='red'>" + QString::number(latestPrice) + "</font>");
        //            } else {
        //                ui->LastPriceLabel->setText("<font color='black'>" + QString::number(latestPrice) + "</font>");
        //            }
        //        }

        //        if (m_open_price_list[m_current_symbol]) {
        //            double diffFromOpen = latestPrice - m_open_price_list[m_current_symbol];
        //            if (diffFromOpen > 0) {
        //                ui->PriceDifferenceLabel->setText("<font color='green'>" + QString::number(abs(diffFromOpen)) + "</font>");
        //                ui->DiffPercentLabel->setText("<font color='green'>(" + QString::number(abs(diffFromOpen) / m_open_price_list[m_current_symbol] * 100, 'f', 2) + "%)" + "</font>");
        //                ui->ArrowLabel->setText("<font color='green'>↑</font>");
        //            } else if (diffFromOpen < 0) {
        //                ui->PriceDifferenceLabel->setText("<font color='red'>" + QString::number(abs(diffFromOpen)) + "</font>");
        //                ui->DiffPercentLabel->setText("<font color='red'>(" + QString::number(abs(diffFromOpen) / m_open_price_list[m_current_symbol] * 100, 'f', 2) + "%)" + "</font>");
        //                ui->ArrowLabel->setText("<font color='red'>↓</font>");
        //            } else {
        //                ui->PriceDifferenceLabel->setText("<font color='black'>" + QString::number(abs(diffFromOpen)) + "</font>");
        //                ui->DiffPercentLabel->setText("<font color='black'>(" + QString::number(abs(diffFromOpen) / m_open_price_list[m_current_symbol] * 100, 'f', 2) + "%)" + "</font>");
        //                ui->ArrowLabel->setText("<font color='black'>-</font>");
        //            }
        //        }
    }
}

/** 
 * @brief Called when user hit the selection and enter OrderBookDialog.
 *        Retrieve the last user selection if exists.
 */
void OrderBookDialog::resume()
{
    setStocklist(Global::qt_core_client.getStocklist());
    m_timer->start();

    if (m_last_clicked_index == 0) {
        ui->StockList->selectRow(0);
        m_current_symbol = ui->StockList->indexAt(QPoint(0, 0)).data().toString();
    } else {
        ui->StockList->selectRow(m_last_clicked_index);
    }

    show();
}

OrderBookDialog::~OrderBookDialog()
{
    delete ui;
}

/**
 * @brief This function update the orderbook when user select a different ticker
 *        from the listview. It unsubscribe the previous showing chart and re-subscribe
 *        the chart for the new ticker.
 */
void OrderBookDialog::onStockListIndexChanged(const QModelIndex& index)
{
    // click on list view to change subscribe
    QString qsymbol = index.data().toString();
    if (qsymbol != m_current_symbol) {
        m_global_bid_data.resetInternalData();
        m_global_ask_data.resetInternalData();
        m_local_bid_data.resetInternalData();
        m_local_ask_data.resetInternalData();

        m_current_symbol = qsymbol;
        m_last_clicked_index = index.row();
    }

    ui->SearchSymbol->setText("");
}

/** 
 * @brief Method to set current stocklist into the tableview
 * @param QStringlist: the stocklist to be set.
 */
void OrderBookDialog::setStocklist(QStringList stocklist)
{
    m_stock_list_model->setStringList(stocklist);
}

void OrderBookDialog::receiveOpenPrice(QString symbol, double openPrice)
{
    m_open_price_list[symbol] = openPrice;
}
