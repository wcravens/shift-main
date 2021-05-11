#include "include/chartdialog.h"
#include "ui_chartdialog.h"

#include "include/qtcoreclient.h"

#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QRegExp>
#include <QTimeZone>

// QWT
#include "qwt_scale_widget.h"

ChartDialog::ChartDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ChartDialog)
    , m_candle_plot(new CandlePlot(this))
    , m_navigation_plot(new NavigationPlot(this))
    , m_wait_label(new QLabel("Please wait while candlestick data is loading...", this))
{
    ui->setupUi(this);

    // maximum
    setWindowFlags(Qt::Window);
    this->setWindowTitle("Candlestick Chart - SHIFT Beta");

    QFont labelFont;
    labelFont.setPointSize(30);
    m_wait_label->setFont(labelFont);

    ui->ChartLayout->addWidget(m_wait_label);
    ui->ChartLayout->setAlignment(m_wait_label, Qt::AlignCenter);
    ui->ChartLayout->addWidget(m_candle_plot);
    ui->ChartLayout->addWidget(m_navigation_plot);

    // init options combo box to switch time interval.
    QStringList intervalOptions;
    intervalOptions.append("1 Sec");
    intervalOptions.append("3 Sec");
    intervalOptions.append("5 Sec");
    intervalOptions.append("10 Sec");
    intervalOptions.append("15 Sec");
    intervalOptions.append("30 Sec");
    intervalOptions.append("1 Min");
    intervalOptions.append("3 Min");
    intervalOptions.append("5 Min");
    intervalOptions.append("10 Min");

    ui->FrequencyOptions->addItems(intervalOptions);
    m_current_interval_index = 0;

    m_interval_options += 1000;
    m_interval_options += 3000;
    m_interval_options += 5000;
    m_interval_options += 10000;
    m_interval_options += 15000;
    m_interval_options += 30000;
    m_interval_options += 60000;
    m_interval_options += 180000;
    m_interval_options += 300000;
    m_interval_options += 600000;

    setTimePeriod(0);
    connect(ui->FrequencyOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(setTimePeriod(int)));

    // initialize the combo box for all zoom options
    QStringList zoomOptions;
    zoomOptions.append("All");
    zoomOptions.append("1 Min");
    zoomOptions.append("10 Min");
    zoomOptions.append("30 Min");
    zoomOptions.append("1 Hour");

    ui->ZoomOptions->addItems(zoomOptions);
    m_current_zoom_index = 0;

    m_zoom_options += 0;
    m_zoom_options += 60000;
    m_zoom_options += 600000;
    m_zoom_options += 1800000;
    m_zoom_options += 3600000;

    connect(ui->ZoomOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(setZoomLevel(int)));

    // init stock list
    m_stock_list_model = new QStringListModel;

    // initialize sort/filter model
    m_filter_model = new StocklistFilterModel;
    m_filter_model->setSourceModel(m_stock_list_model);
    m_filter_model->setFilterKeyColumn(1);
    connect(ui->SearchSymbol, &QLineEdit::textChanged, m_filter_model, &StocklistFilterModel::setFilterText);

    ui->StockList->setModel(m_filter_model);
    connect(ui->StockList, &QTableView::clicked, this, &ChartDialog::onStockListIndexChanged);

    // All plot related connections
    connect(&Global::qt_core_client, &QtCoreClient::updateCandleChart, this, &ChartDialog::receiveData);
    connect(m_navigation_plot, &NavigationPlot::timeFrameSelected, this, &ChartDialog::updateXAxis);
    connect(m_candle_plot, &CandlePlot::intervalChanged, this, &ChartDialog::updateIntervalSelection);
    connect(m_candle_plot, &CandlePlot::zoomChanged, this, &ChartDialog::updateZoomSelection);
    connect(m_candle_plot, &CandlePlot::updateMarker, this, &ChartDialog::updateMarker);

    // set interval for refresh timer
    m_timer.setInterval(500); // millisecond

    m_is_loading_done = false;
}

/** 
 * @brief Method to refresh the chart as new data coming in.
 */
void ChartDialog::refresh()
{
    if (!m_candle_plot->isDataReady(m_current_symbol))
        return;
    m_candle_plot->refresh(m_current_symbol);
    m_navigation_plot->refresh(m_current_symbol);
}

/** 
 * @brief Called when user hit the selection and enter ChartDialog.
 *        Retrieve the last user selection if exists.
 */
void ChartDialog::start()
{
    // return QStringList
    setStocklist(Global::qt_core_client.getStocklist());

    // return vector of string
    for (std::string name : Global::qt_core_client.getStockList())
        m_current_stocklist.push_back(QString::fromStdString(name));

    // Save the previously selected position.
    if (m_last_clicked_index == 0) {
        ui->StockList->selectRow(0);
        //        QString selected_symbol = ui->StockList->indexAt(QPoint(0, 0)).data().toString();
        m_current_symbol = ui->StockList->indexAt(QPoint(0, 0)).data().toString();
        ;
    } else {
        ui->StockList->selectRow(m_last_clicked_index);
    }

    initializeChart();

    show();
}

/**
 * @brief Called when the dialog first opened or resumed. Clear the chart first then subscribe the 
 *        chart for the selected ticker.
 */
void ChartDialog::initializeChart()
{
    QTimeZone current = QDateTime::currentDateTime().timeZone();
    QString location = QString::fromStdString(current.id().toStdString());

    QRegExp findCity("^.+/");
    location.replace(QRegExp("^.+/"), "");
    location.replace(QRegExp("_"), " ");

    ui->TimeZoneLabel->setText("Local Time Zone: " + location);

    // reconnect receive data functions and timer
    connect(&m_timer, &QTimer::timeout, this, &ChartDialog::refresh);

    // restart the timer
    m_timer.start();
}

/**QDialog
 * @brief Method to check whether the ticker data is from Nasdaq.
 * @param std::string: ticker name to be checked.
 * @return bool: true if it's from nasdaq, vice versa.
 */
bool ChartDialog::isFromNasdaq(QString name)
{
    QRegExp test(".O");
    int pos = test.indexIn(name);

    if (pos >= 0)
        return true;
    else
        return false;
}

/** 
 * @brief Called when the selected time period changes.
 * @param int: the index for the newly selected time period.
 */
void ChartDialog::setTimePeriod(const int& index)
{
    if (!m_candle_plot->isDataReady(m_current_symbol)) {
        m_wait_label->setHidden(false);
        m_candle_plot->setHidden(true);
        m_navigation_plot->setHidden(true);
        ui->Zoom->setHidden(true);
        ui->ZoomOptions->setHidden(true);
        ui->TimeZoneLabel->setHidden(true);
        ui->Frequency->setHidden(true);
        ui->FrequencyOptions->setHidden(true);
        return;
    }

    m_wait_label->setHidden(true);
    m_candle_plot->setHidden(false);
    m_navigation_plot->setHidden(false);
    ui->Zoom->setHidden(false);
    ui->ZoomOptions->setHidden(false);
    ui->TimeZoneLabel->setHidden(false);
    ui->Frequency->setHidden(false);
    ui->FrequencyOptions->setHidden(false);

    if (!index && !m_current_interval_index) {
        m_candle_plot->setInterval(m_interval_options[m_current_interval_index]);
    } else if (index != m_current_interval_index) {
        QMutexLocker lock(&m_mutex);

        // reset index and data set
        m_current_interval_index = index;
        m_candle_plot->setInterval(m_interval_options[m_current_interval_index]);
        m_candle_plot->clearData(m_current_symbol);

        if (!m_raw_samples[m_current_symbol].empty()) {
            // provide data with time frequency for plot
            long long interval = m_interval_options[index];
            long long last_timestamp = m_raw_samples[m_current_symbol][0].m_timestamp;
            last_timestamp = last_timestamp - last_timestamp % interval;
            double last_open = m_raw_samples[m_current_symbol][0].m_open;
            double last_high = m_raw_samples[m_current_symbol][0].m_high;
            double last_low = m_raw_samples[m_current_symbol][0].m_low;
            double last_close = m_raw_samples[m_current_symbol][0].m_close;

            for (size_t i = 1; i < m_raw_samples[m_current_symbol].size(); ++i) {
                CandleDataSample sample = m_raw_samples[m_current_symbol][i];
                if (sample.m_timestamp - last_timestamp < interval) {
                    last_high = qMax(last_high, sample.m_high);
                    last_low = qMin(last_low, sample.m_low);
                    last_close = sample.m_close;
                } else {
                    // when reach a complete time interval, send to candlePlot to draw one candle
                    m_candle_plot->receiveData(m_current_symbol, last_timestamp, last_open, last_high, last_low, last_close);
                    last_timestamp += interval;
                    last_open = sample.m_open;
                    last_high = sample.m_high;
                    last_low = sample.m_low;
                    last_close = sample.m_close;
                }
            }

            // last data set
            m_candle_plot->receiveData(m_current_symbol, last_timestamp, last_open, last_high, last_low, last_close);
            refresh();
        }
    }
}

/** 
 * @brief Method to set current stocklist into the tableview
 * @param QStringlist: the stocklist to be set.
 */
void ChartDialog::setStocklist(const QStringList& stocklist)
{
    m_stock_list_model->setStringList(stocklist);
}

ChartDialog::~ChartDialog()
{
    delete ui;
}

/*
 * @brief this function update the chart when user select a different ticker
 *        from the listview. It unsubscribe the previous showing chart and re-subscribe
 *        the chart for the new ticker.
 * @param const QModelIndex&: the index for the selected ticker.
 */
void ChartDialog::onStockListIndexChanged(const QModelIndex& index)
{
    /**
     * FYI
     * QMutexLocker lock the specific mutex when it's created and automatically unlock it
     * when exit the function.
     */
    // click on list view to change subscribe
    QString qsymbol = index.data().toString();

    if (qsymbol != m_current_symbol) {
        m_current_symbol = qsymbol;

        int tempIndex = m_current_interval_index;
        m_current_interval_index = -1;
        setTimePeriod(tempIndex);
        ui->FrequencyOptions->setCurrentIndex(tempIndex);

        m_last_clicked_index = index.row();
    }

    ui->SearchSymbol->setText("");
}

/**
 * @brief Called when updateCandleData() signal is emitted from QtCoreClient. It calls receiveData method for candleplot.
 * @param QString: symbol of the updated data.
 * @param long long: timestamp of the updated data.
 * @param double: open/high/low/close of the updated data.
 */
void ChartDialog::receiveData(const QString& symbol, const long long& timestamp, const double& open, const double& high, const double& low, const double& close)
{
    CandleDataSample sample(timestamp, open, high, low, close);
    m_raw_samples[symbol].push_back(sample);

    m_candle_plot->receiveData(symbol, timestamp, open, high, low, close);

    double avg = (open + high + low + close) / 4;
    m_navigation_plot->receiveData(symbol, timestamp, avg);

    QString firstSymbol = QString::fromStdString(Global::qt_core_client.getStockList().front());
    QString lastSymbol = QString::fromStdString(Global::qt_core_client.getStockList().back());

    //    qDebug() << m_raw_samples[firstSymbol].size() << "\t" << m_raw_samples[lastSymbol].size();
    if (m_raw_samples[firstSymbol].size() > m_raw_samples[lastSymbol].size() - 10 && !m_is_loading_done) {
        m_is_loading_done = true;
        emit dataLoaded();
    }
}

/**
 * @brief Accept timeFrameSelected signal and call the updateXAxis for CandlePlot class.
 * @param long long the newly selected time 
 */
void ChartDialog::updateXAxis(const long long& time)
{
    m_candle_plot->updateXAxis(time);
}

/**
 * @brief Accept intervalChanged signal from CandlePlot and update the interval selection for the 
 *        combo box of frequency.
 * @param long long new interval
 */
void ChartDialog::updateIntervalSelection(const long long& interval)
{
    int i;
    // find the index of the new interval in the combo box
    for (i = 0; i < m_interval_options.size(); ++i) {
        if (m_interval_options[i] == interval)
            break;
    }

    if (i == m_interval_options.size())
        i = 0;

    ui->FrequencyOptions->setCurrentIndex(i);
}

/**
 * @brief Method to update markers in nav bar.
 * @param long long starting timestamp
 * @param long long ending timestamp
 */
void ChartDialog::updateMarker(const long long& start, const long long& end)
{
    m_navigation_plot->updateMarker(start, end);
}

/** 
 * @brief Method called when the zoom level is changed. Redirect to the setZoomBlock
 *        in both candleplot and navbar.
 * @param int seleceted index
 */
void ChartDialog::setZoomLevel(const int& index)
{
    m_candle_plot->setZoomBlock(m_zoom_options[index]);
    m_navigation_plot->setZoomBlock(m_zoom_options[index]);
}

/** 
 * @brief Method called when the zoom level is changed outside chartdialog class
 * @param long long new zoom level selection
 */
void ChartDialog::updateZoomSelection(const long long& interval)
{
    int i;
    for (i = 0; i < m_zoom_options.size(); ++i) {
        if (m_zoom_options[i] == interval)
            break;
    }

    if (i == m_zoom_options.size())
        i = 0;

    ui->ZoomOptions->setCurrentIndex(i);
}
