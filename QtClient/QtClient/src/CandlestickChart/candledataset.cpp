#include "include/CandlestickChart/candledataset.h"


CandleDataSet::CandleDataSet()
{
}

CandleDataSet::CandleDataSet(QString symbol)
    : m_symbol(symbol)
{
}

bool CandleDataSet::getSampleByTimestamp(long long timestamp, QwtOHLCSample& sample)
{
    if (d_samples.size() > 1) {
        long long firstTime = d_samples[0].time;
        long long timeInterval = d_samples[1].time - firstTime;
        int index = (timestamp - firstTime + timeInterval / 2) / timeInterval;
        if (index >= 0 && index < d_samples.size()) {
            sample = d_samples[index];
            return true;
        }
    }
    return false;
}
