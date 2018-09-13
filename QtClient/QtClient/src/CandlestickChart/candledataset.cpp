#include "include/CandlestickChart/candledataset.h"


CandleDataSet::CandleDataSet()
{
}

CandleDataSet::CandleDataSet(QString symbol)
    : m_symbol(symbol)
{
}

/**
 * @brief Method to return a QwtOHLCSample(candledatasample) by looking for the
 *        target timestamp. (Only used when show tracker text)
 * @param timestamp The timestamp of the target sample in long long type.
 * @param sample The address/reference of the returned sample.
 * @return bool If the sample is found, return true, otherwise return false.
 */
bool CandleDataSet::getSampleByTimestamp(long long timestamp, QwtOHLCSample& sample)
{
    if (d_samples.size() > 1) {
        long long firstTime = d_samples[0].time;
        long long timeInterval = d_samples[1].time - d_samples[0].time;
        int index = (timestamp - firstTime + timeInterval / 2) / timeInterval;
        if (index >= 0 && index < d_samples.size()) {
            sample = d_samples[index];
            return true;
        }
    }
    return false;
}
