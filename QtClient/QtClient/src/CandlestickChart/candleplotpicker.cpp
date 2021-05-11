#include "include/CandlestickChart/candleplotpicker.h"

#include <QDateTime>

// QWT
#include <qwt_series_data.h>

/*
 * @brief This function is used to update/set the data_set for current
 *        PlotPicker.
 * @param CandleDataSet*: the data_set to be set into the plot picker.
 */
void CandlePlotPicker::setCandleDataSet(CandleDataSet* data_set)
{
    m_candle_data_set = data_set;
}

/*
 * @brief This function implement the tracking text box panning above
 *        the plot.
 * @param QPointF&: the point position in the plot to be tracked.
 * @return QwtText: Detail info about that point including Open, High,
 *         Low, and Close.
 */
QwtText CandlePlotPicker::trackerTextF(const QPointF& pos) const
{
    emit mouseMoved(pos);

    if (m_candle_data_set != NULL) {
        long long timestamp = pos.x();
        QwtOHLCSample sample;
        if (m_candle_data_set->getSampleByTimestamp(timestamp, sample)) {
            QString text;
            text.append(QString(QDateTime::fromMSecsSinceEpoch(timestamp, Qt::LocalTime).toString("dddd, MMMM dd, hh:mm:ss") + "\n"));
            text.append(QString("Open  %1\n").arg(sample.open, 2));
            text.append(QString("High  %1\n").arg(sample.high, 2));
            text.append(QString("Low   %1\n").arg(sample.low, 2));
            text.append(QString("Close %1").arg(sample.close, 2));
            return QwtText(text);
        }
    }

    return QwtText("");
}
