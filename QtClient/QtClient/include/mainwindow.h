#pragma once

#include "include/chartdialog.h"
#include "include/orderbookdialog.h"
#include "include/overviewmodel.h"
#include "include/portfoliomodel.h"
#include "include/waitinglistmodel.h"

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

/**
 * @brief Class which defined the main(overview) dialog of the application, including .cpp and .ui,
 *        extends QDialog.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    Ui::MainWindow* ui;

    void submitOrder(shift::Order::Type type);

public slots:
    void updatePortfolio(double totalBP, int totalShare);
    void updatePL(double totalPL);
    void updateOrderEditor(QString symbol, double price);
    void updateCancelOrderEditor(QString id, QString size);
    void hideLoadingLabel();
    void onThemeActionClicked();

private:
    ChartDialog *m_chart_dialog;
    OrderBookDialog *m_order_book_dialog;
    OverviewModel m_overview_model;
    PortfolioModel m_portfolio_model;
    WaitingListModel m_waiting_list_model;

    void setTheme(const QString &path);

private slots:
    void on_actionCandlestick_Chart_triggered();
    void on_actionShow_Order_Book_triggered();
    void on_LimitBuyButton_clicked();
    void on_LimitSellButton_clicked();
    void on_MarketBuyButton_clicked();
    void on_MarketSellButton_clicked();
    void on_CancelOrderButton_clicked();
    void on_CancelAllButton_clicked();
};
