#include "database/Tables.h"

namespace shift::database {

// in C++, if outside member definition has been qualified by any complete template type, e.g. "A<X>::",
// and there is no specialization "template<> class/struct A<X> {...};" ever declared,
// then there always shall be a "template<>" before each such definition --
// otherwise, there shall NOT any "template<>" present.

/* static */ constexpr char PSQLTable<TradeAndQuoteRecords>::sc_colsDefinition[];
/* static */ constexpr char PSQLTable<TradeAndQuoteRecords>::sc_recordFormat[];

/* static */ constexpr char PSQLTable<TradingRecords>::sc_colsDefinition[];
/* static */ const char* PSQLTable<TradingRecords>::name = "trading_records";

/* static */ constexpr char PSQLTable<PortfolioSummary>::sc_colsDefinition[];
/* static */ const char* PSQLTable<PortfolioSummary>::name = "portfolio_summary";

/* static */ constexpr char PSQLTable<PortfolioItem>::sc_colsDefinition[];
/* static */ const char* PSQLTable<PortfolioItem>::name = "portfolio_items";

/* static */ constexpr char PSQLTable<Leaderboard>::sc_colsDefinition[];
/* static */ const char* PSQLTable<Leaderboard>::name = "leaderboard";

} // shift::database
