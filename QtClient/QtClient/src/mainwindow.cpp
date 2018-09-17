#include "include/mainwindow.h"
#include "ui_mainwindow.h"

#include "include/qtcoreclient.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_chart_dialog(new ChartDialog(this))
    , m_order_book_dialog(new OrderBookDialog(this))
{
    ui->setupUi(this);
    this->setWindowTitle("Overview - SHIFT Alpha");
    this->setAttribute(Qt::WA_QuitOnClose);

    // set up model
    ui->OverviewTable->setModel(&m_overview_model);
    ui->OverviewTable->horizontalHeader()->setFixedHeight(50);
    ui->PortfolioTable->setModel(&m_portfolio_model);
    ui->WaitingListTable->setModel(&m_waiting_list_model);
    ui->actionCandlestick_Chart->setEnabled(false);

    // set table column width
    for (int i = 0; i < m_overview_model.columnCount(QModelIndex()); i++) {
        ui->OverviewTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    for (int i = 0; i < m_portfolio_model.columnCount(QModelIndex()); i++) {
        ui->PortfolioTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    for (int i = 0; i < m_waiting_list_model.columnCount(QModelIndex()); i++) {
        ui->WaitingListTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }

    // connect signal and slot
    connect(&Global::qt_core_client, &QtCoreClient::stocklistReady, &m_overview_model, &OverviewModel::receiveStocklistReady);
    connect(&Global::qt_core_client, &QtCoreClient::updatePortfolio, &m_portfolio_model, &PortfolioModel::receivePortfolio);
    connect(&Global::qt_core_client, &QtCoreClient::updateWaitingList, &m_waiting_list_model, &WaitingListModel::receiveWaitingList);
    connect(&m_portfolio_model, &PortfolioModel::updateTotalPortfolio, this, &MainWindow::updatePortfolio);
    connect(&m_portfolio_model, &PortfolioModel::updateTotalPL, this, &MainWindow::updatePL);
    connect(ui->OverviewTable, &QTableView::clicked, &m_overview_model, &OverviewModel::onClicked);
    connect(ui->WaitingListTable, &QTableView::clicked, &m_waiting_list_model, &WaitingListModel::onClicked);
    connect(&m_waiting_list_model, &WaitingListModel::setCancelOrder, this, &MainWindow::updateCancelOrderEditor);
    connect(m_chart_dialog, &ChartDialog::dataLoaded, this, &MainWindow::hideLoadingLabel);
    connect(&m_overview_model, &OverviewModel::setSendOrder, this, &MainWindow::updateOrderEditor);
    connect(&m_overview_model, &OverviewModel::sentOpenPrice, m_order_book_dialog, &OrderBookDialog::receiveOpenPrice);
}

MainWindow::~MainWindow()
{
    qDebug()<<"MainWindow::~MainWindow()";
    delete ui;
}

/**
 * @brief Validify all information and call QtCoreClient to submit the order to BrokerageCenter.
 */
void MainWindow::submitOrder(shift::Order::ORDER_TYPE type)
{
    QString qsymbol = ui->Symbol->toPlainText().toUpper();

    // check symbol
    if (!Global::qt_core_client.getStocklist().contains(qsymbol, Qt::CaseInsensitive)) {
        QMessageBox msg;
        msg.setText("Symbol " + qsymbol + " doesn't exist.");
        msg.exec();
        return;
    }

    // check price
    double price = 1;
    // if it's not Market Buy or Market Sell, get price from user input
    if (type != shift::Order::MARKET_BUY && type != shift::Order::MARKET_SELL) {
        price = ui->Price->toPlainText().toDouble();
    }
    double price100 = price * 100;
    int round_price = round(price100);
    if (abs(price100 - round_price) > 0.01 || price <= 0.0) {
        qDebug() << price100 << "\t" << round_price << price;
        QMessageBox msg;
        msg.setText("Price " + ui->Price->toPlainText() + " is invalid.");
        msg.exec();
        return;
    }
    price = round_price / 100;

    // check size
    int size = ui->Size->toPlainText().toInt();
    if (size <= 0) {
        QMessageBox msg;
        msg.setText("Size " + ui->Size->toPlainText() + " is invalid.");
        msg.exec();
        return;
    }

    shift::Order order(qsymbol.toStdString(), price, size, type);
    Global::qt_core_client.submitOrder(order);
}

void MainWindow::updatePortfolio(double totalBP, int totalShare)
{
    ui->BuyingPower->setText(QString::number(totalBP, 'f', 2));
    ui->TotalShare->setText(QString::number(totalShare));
}

void MainWindow::updatePL(double totalPL)

{
    ui->TotalPL->setText(QString::number(totalPL, 'f', 2));
}

void MainWindow::updateOrderEditor(QString symbol, double price)
{
    ui->Symbol->setText(symbol);
    ui->Price->setText(QString::number(price, 'f', 2));
    ui->Size->setText(QString::number(1));
}

//! Inefficient funciton naming and excessive comments.
///**
// * @brief Method called when waitinglist QTableView is clicked. Set clicked
// *        info into the TextEdit field.
// * @param QString: ID of the order to be cancelled.
// * @param QString: size of the order.
// */
//void MainWindow::receiveCancelOrderSetting(QString id, QString size)
//{
//    ui->CancelOrderID->setText(id);
//    ui->CancelOrderSize->setText(size);
//}

//! Efficient funtion naming of the same funciton.
void MainWindow::updateCancelOrderEditor(QString id, QString size)
{
    ui->CancelOrderID->setText(id);
    ui->CancelOrderSize->setText(size);
}

/**
 * @brief Open ChartDialog when the corresponding button is clicked.
 */
void MainWindow::on_actionCandlestick_Chart_triggered()
{
    m_chart_dialog->start();
}

/**
 * @brief Open OrderBookDialog when the corresponding button is clicked.
 */
void MainWindow::on_actionShow_Order_Book_triggered()
{
    m_order_book_dialog->resume();
}

void MainWindow::on_LimitBuyButton_clicked()
{
    submitOrder(shift::Order::LIMIT_BUY);
}

void MainWindow::on_LimitSellButton_clicked()
{
    submitOrder(shift::Order::LIMIT_SELL);
}

void MainWindow::on_MarketBuyButton_clicked()
{
    submitOrder(shift::Order::MARKET_BUY);
}

void MainWindow::on_MarketSellButton_clicked()
{
    submitOrder(shift::Order::MARKET_SELL);
}

void MainWindow::on_CancelOrderButton_clicked()
{
    //! Send a cancelBid/cancelAsk order (type = 5/6)
    shift::Order q;
    if (!m_waiting_list_model.getCancelOrderByID(ui->CancelOrderID->toPlainText(), q)) {
        QMessageBox msg;
        msg.setText("Order ID " + ui->CancelOrderID->toPlainText() + " does not exist");
        msg.exec();
        return;
    }

    int size = ui->CancelOrderSize->toPlainText().toInt();
    if (size <= 0 || size > q.getSize()) {
        QMessageBox msg;
        msg.setText("Size " + ui->CancelOrderSize->toPlainText() + " is invalid");
        msg.exec();
    } else {
        q.setSize(size);
        Global::qt_core_client.submitOrder(q);
    }
    ui->CancelOrderID->setText("");
    ui->CancelOrderSize->setText("");
}

void MainWindow::on_CancelAllButton_clicked()
{
    std::vector<shift::Order> order_list = m_waiting_list_model.getAllOrders();
    for (shift::Order order : order_list)
        Global::qt_core_client.submitOrder(order);
}

/**
 * @brief When candlestick data loading is done, hide the loading message
 */
void MainWindow::hideLoadingLabel()
{
    ui->loadingLabel->hide();
//    ui->menuChart->setVisible(true);'
//    QMenu* chartMenu = new QMenu("Charts", this);
//    chartMenu->addAction("Candlestick Charts", this, SLOT(on_actionShow_Chart_triggered()));
//    ui->menuBar->addMenu(chartMenu);
    ui->actionCandlestick_Chart->setEnabled(true);
}
