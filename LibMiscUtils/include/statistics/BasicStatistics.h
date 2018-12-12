#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>

namespace shift {
namespace statistics {

    template <typename _Values>
    double mean(const _Values& data)
    {
        if (data.begin() == data.end()) {
            return 0.0;
        }

        return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    }

    template <typename _XValues>
    double variance(const _XValues& xdata)
    {
        if (xdata.begin() == xdata.end()) {
            return 0.0;
        }

        _XValues xdiff(xdata.size());
        double xbar = mean(xdata);

        std::transform(xdata.begin(), xdata.end(), xdiff.begin(), [xbar](double x) { return x - xbar; });

        return std::inner_product(xdiff.begin(), xdiff.end(), xdiff.begin(), 0.0) / xdiff.size();
    }

    template <typename _Values>
    double stddev(const _Values& data)
    {
        if (data.begin() == data.end()) {
            return 0.0;
        }

        return std::sqrt(variance(data));
    }

    template <typename _XValues, typename _YValues>
    double covariance(const _XValues& xdata, const _YValues& ydata)
    {
        if ((xdata.begin() == xdata.end()) || (xdata.size() != ydata.size())) {
            return 0.0;
        }

        _XValues xdiff(xdata.size());
        double xbar = mean(xdata);

        std::transform(xdata.begin(), xdata.end(), xdiff.begin(), [xbar](double x) { return x - xbar; });

        _YValues ydiff(ydata.size());
        double ybar = mean(ydata);

        std::transform(ydata.begin(), ydata.end(), ydiff.begin(), [ybar](double y) { return y - ybar; });

        return std::inner_product(xdiff.begin(), xdiff.end(), ydiff.begin(), 0.0) / xdiff.size();
    }

    template <typename _XValues, typename _YValues>
    double correlation(const _XValues& xdata, const _YValues& ydata)
    {
        if ((xdata.begin() == xdata.end()) || (xdata.size() != ydata.size())) {
            return 0.0;
        }

        return covariance(xdata, ydata) / (stddev(xdata) * stddev(ydata));
    }

} // statistics
} // shift
