#include "database/Tables.h"

namespace shift {
namespace database {

    /*static*/ constexpr char PSQLTable<TradingRecords>::sc_colsDefinition[];
    /*static*/ constexpr char PSQLTable<TradingRecords>::sc_recordFormat[];
    /*static*/ const char* PSQLTable<TradingRecords>::name = "trading_records";

    /*static*/ constexpr char PSQLTable<PortfolioSummary>::sc_colsDefinition[];
    /*static*/ constexpr char PSQLTable<PortfolioSummary>::sc_recordFormat[];
    /*static*/ const char* PSQLTable<PortfolioSummary>::name = "portfolio_summary";

    /*static*/ constexpr char PSQLTable<PortfolioItem>::sc_colsDefinition[];
    /*static*/ constexpr char PSQLTable<PortfolioItem>::sc_recordFormat[];
    /*static*/ const char* PSQLTable<PortfolioItem>::name = "portfolio_items";

} // database
} // shift
