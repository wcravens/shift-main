#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>

namespace shift::statistics {

template <typename _Values>
auto mean(const _Values& data) -> double
{
    if (data.begin() == data.end()) {
        return 0.0;
    }

    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

template <typename _XValues>
auto variance(const _XValues& xdata) -> double
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
auto stddev(const _Values& data) -> double
{
    if (data.begin() == data.end()) {
        return 0.0;
    }

    return std::sqrt(variance(data));
}

template <typename _XValues, typename _YValues>
auto covariance(const _XValues& xdata, const _YValues& ydata) -> double
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
auto correlation(const _XValues& xdata, const _YValues& ydata) -> double
{
    if ((xdata.begin() == xdata.end()) || (xdata.size() != ydata.size())) {
        return 0.0;
    }

    return covariance(xdata, ydata) / (stddev(xdata) * stddev(ydata));
}

template <typename _Returns>
auto realizedVariance(const _Returns& data) -> double
{
    if (data.begin() == data.end()) {
        return 0.0;
    }

    return std::inner_product(data.begin(), data.end(), data.begin(), 0.0);
}

template <typename _Returns>
auto realizedVolatility(const _Returns& data) -> double
{
    if (data.begin() == data.end()) {
        return 0.0;
    }

    return std::sqrt(realizedVariance(data));
}

} // shift::statistics
