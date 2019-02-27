#include "database/Tables.h"

namespace shift {
namespace database {

    /*static*/ constexpr char PSQLTable<TradeAndQuoteRecords>::sc_colsDefinition[]; // In C++, if outside member definition has been qualified by any complete template type, e.g. "A<X>::", and there is no specialization "template<> class/struct A<X> {...};" ever declared, then there always shall be a "template<>" before each such definition. Otherwise, there shall NOT any "template<>" present.
    /*static*/ constexpr char PSQLTable<TradeAndQuoteRecords>::sc_recordFormat[];

    /*static*/ constexpr char PSQLTable<NamesOfTradeAndQuoteTables>::sc_colsDefinition[];
    /*static*/ const char* PSQLTable<NamesOfTradeAndQuoteTables>::name = "list_of_taq_tables";

    /*static*/ constexpr char PSQLTable<DETradingRecords>::sc_colsDefinition[];
    /*static*/ const char* PSQLTable<DETradingRecords>::name = "trading_records";

    /*static*/ constexpr char PSQLTable<BCTradingRecords>::sc_colsDefinition[];
    /*static*/ constexpr char PSQLTable<BCTradingRecords>::sc_recordFormat[];
    /*static*/ const char* PSQLTable<BCTradingRecords>::name = "trading_records";

    /*static*/ constexpr char PSQLTable<PortfolioSummary>::sc_colsDefinition[];
    /*static*/ constexpr char PSQLTable<PortfolioSummary>::sc_recordFormat[];
    /*static*/ const char* PSQLTable<PortfolioSummary>::name = "portfolio_summary";

    /*static*/ constexpr char PSQLTable<PortfolioItem>::sc_colsDefinition[];
    /*static*/ constexpr char PSQLTable<PortfolioItem>::sc_recordFormat[];
    /*static*/ const char* PSQLTable<PortfolioItem>::name = "portfolio_items";

} // database
} // shift