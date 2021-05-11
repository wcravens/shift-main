#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QString>

// QWT
#include <qwt_series_data.h>

/**
 * @brief Class to save data to use in the candleplot.
 * QwtArraySeriesData: Template class for data, organized as QVector.
 * QwtOHLCSample: Open-High-Low-Close sample used in financial charts.
 *     Attributes: double time, double open, double high, double low,
 *                 double close.
 */
class CandleDataSet {
public:
    QString m_symbol;
    CandleDataSet();
    CandleDataSet(QString symbol);
    QVector<QwtOHLCSample> d_samples;

    bool getSampleByTimestamp(long long, QwtOHLCSample&);

    /**
     * @brief Inline function to append new sample into the sample list.
     */
    inline void append(const QwtOHLCSample& sample)
    {
        /**
         * d_samples is a protected attributes inherited from 
         * QwtArraySeriesData. Vector of samples.
         */
        d_samples.push_back(std::move(sample));
    }

    /**
     * @brief Inline function to update/insert new data/sample into the sample
     *        list.
     */
    inline void update(const QwtOHLCSample& sample)
    {
        if (!d_samples.empty()) {
            QwtOHLCSample& last = d_samples.back();
            d_samples.pop_back();
            last.high = qMax(last.high, sample.high);
            last.low = qMin(last.low, sample.low);
            last.close = sample.close;
            d_samples.push_back(std::move(last));
        } else {
            d_samples.push_back(std::move(sample));
        }
    }
};
