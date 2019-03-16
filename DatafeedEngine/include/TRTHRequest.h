#pragma once

#include <future>
#include <string>

/**@brief Parameter table for one TRTH request.
 *        It is the media for other parts communicating with TRTHAPI.
 */
struct TRTHRequest {
    TRTHRequest() = default; // TRTHRequest is an aggregate (i.e. like C-style struct)
    // Other defaulted special members shall be available ...

    std::string symbol;
    std::string date; // Date format: YYYY-MM-DD
    std::promise<bool>* prom; ///> For signaling the completion of this request. bool value indicates whether it was a smooth, successful processing.
};
