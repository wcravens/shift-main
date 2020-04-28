#pragma once

namespace shift::database {

/**
     * @brief Tag of table for storing Trade and Quote records.
     */
struct TradeAndQuoteRecords;

/**
     * @brief Tag of table for storing Trading records.
     */
struct TradingRecords;

struct PortfolioSummary;
struct PortfolioItem;
struct Leaderboard;

//----------------------------------------------------------------------------------------------------------

template <typename>
struct PSQLTable;

//----------------------------------------------------------------------------------------------------------

template <>
struct PSQLTable<TradeAndQuoteRecords> {
    static constexpr char sc_colsDefinition[] = "( ric VARCHAR(15)"
                                                ", reuters_date DATE"
                                                ", reuters_time TIME"
                                                ", reuters_time_order INTEGER"
                                                ", reuters_time_offset SMALLINT"
                                                ", toq CHAR" /* trade or quote */
                                                ", exchange_id VARCHAR(10)"
                                                ", price REAL"
                                                ", volume INTEGER"
                                                ", buyer_id VARCHAR(10)"
                                                ", bid_price REAL"
                                                ", bid_size INTEGER"
                                                ", seller_id VARCHAR(10)"
                                                ", ask_price REAL"
                                                ", ask_size INTEGER"
                                                ", exchange_time TIME"
                                                ", quote_time TIME"

                                                ", PRIMARY KEY (reuters_time, reuters_time_order) )";

    static constexpr char sc_recordFormat[] = "  ric"
                                              ", reuters_date"
                                              ", reuters_time"
                                              ", reuters_time_order"
                                              ", reuters_time_offset"
                                              ", toq"
                                              ", exchange_id"
                                              ", price"
                                              ", volume"
                                              ", buyer_id"
                                              ", bid_price"
                                              ", bid_size"
                                              ", seller_id"
                                              ", ask_price"
                                              ", ask_size"
                                              ", exchange_time"
                                              ", quote_time";

    enum VAL_IDX : int {
        RIC = 0,
        REUT_DATE,
        REUT_TIME,
        RT_ORDER,
        RT_OFFSET,
        TOQ,
        EXCH_ID,
        PRICE,
        VOLUMN,
        BUYER_ID,
        BID_PRICE,
        BID_SIZE,
        SELLER_ID,
        ASK_PRICE,
        ASK_SIZE,
        EXCH_TIME,
        QUOTE_TIME,

        NUM_FIELDS
    };
};

//----------------------------------------------------------------------------------------------------------

template <>
struct PSQLTable<TradingRecords> {
    static constexpr char sc_colsDefinition[] = "( session_id VARCHAR(50)"
                                                ", real_time TIMESTAMP"
                                                ", execution_time TIMESTAMP"
                                                ", symbol VARCHAR(15)"
                                                ", price REAL"
                                                ", size INTEGER"
                                                ", trader_id_1 VARCHAR(40)" // == UUID (except "TRTH") which maps to traders.id
                                                ", trader_id_2 VARCHAR(40)" // ditto
                                                ", order_id_1 VARCHAR(40)"
                                                ", order_id_2 VARCHAR(40)"
                                                ", order_type_1 VARCHAR(2)"
                                                ", order_type_2 VARCHAR(2)"
                                                ", time_1 TIMESTAMP"
                                                ", time_2 TIMESTAMP"
                                                ", decision CHAR"
                                                ", destination VARCHAR(10)"

                                                ", CONSTRAINT trading_records_pkey PRIMARY KEY (execution_time, order_id_1, order_id_2) )";

    static const char* name;
};

//----------------------------------------------------------------------------------------------------------

template <>
struct PSQLTable<PortfolioSummary> {
    static constexpr char sc_colsDefinition[] = "( id UUID"
                                                ", buying_power REAL DEFAULT 1e6"
                                                ", holding_balance REAL DEFAULT 0.0"
                                                ", borrowed_balance REAL DEFAULT 0.0"
                                                ", total_pl REAL DEFAULT 0.0"
                                                ", total_shares INTEGER DEFAULT 0"

                                                ", CONSTRAINT portfolio_summary_pkey PRIMARY KEY (id)\
                                                    \
                                                     , CONSTRAINT portfolio_summary_fkey FOREIGN KEY (id)\
                                                        REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                    )";

    static const char* name;
};

//----------------------------------------------------------------------------------------------------------

template <>
struct PSQLTable<PortfolioItem> {
    static constexpr char sc_colsDefinition[] = "( id UUID"
                                                ", symbol VARCHAR(15) DEFAULT '<UnknownSymbol>'"
                                                ", borrowed_balance REAL DEFAULT 0.0"
                                                ", pl REAL DEFAULT 0.0"
                                                ", long_price REAL DEFAULT 0.0"
                                                ", short_price REAL DEFAULT 0.0"
                                                ", long_shares INTEGER DEFAULT 0"
                                                ", short_shares INTEGER DEFAULT 0"

                                                ", CONSTRAINT portfolio_items_pkey PRIMARY KEY (id, symbol)\
                                                    \
                                                     , CONSTRAINT portfolio_items_fkey FOREIGN KEY (id)\
                                                        REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                    )";

    static const char* name;
};

template <>
struct PSQLTable<Leaderboard> {
    static constexpr char sc_colsDefinition[] = "( id UUID DEFAULT uuid_generate_v4()"
                                                ", start_date timestamp without time zone"
                                                ", end_date timestamp without time zone"
                                                ", contest_day integer"
                                                ", rank integer"
                                                ", trader_id UUID NOT NULL"
                                                ", eod_buying_power real"
                                                ", eod_traded_shares integer"
                                                ", eod_pl real"
                                                ", fees real"
                                                ", total_orders integer"
                                                ", penalty_total real"
                                                ", pl_2 real )";
    static const char* name;
};

} // shift::database
