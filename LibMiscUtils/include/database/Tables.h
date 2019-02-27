#pragma once

namespace shift {
namespace database {

    /*@brief Tag of table for storing Trade and Quote records */
    struct TradeAndQuoteRecords;
    /*@brief Tag of table for memorizing (by names) downloaded Trade and Quote tables */
    struct NamesOfTradeAndQuoteTables;
    /*@brief Tag of table for storing Trading records */
    struct DETradingRecords;

    struct BCTradingRecords;
    struct PortfolioSummary;
    struct PortfolioItem;

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

                                                    ", toq CHAR" /*Trade Or Quote*/
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

                                                    ", PRIMARY KEY (reuters_time, reuters_time_order) ) ";

        static constexpr char sc_recordFormat[] = " (ric"
                                                  ", reuters_date, reuters_time, reuters_time_order, reuters_time_offset, toq"
                                                  ", exchange_id,  price,        volume,       buyer_id,           bid_price"
                                                  ", bid_size,     seller_id,    ask_price,    ask_size,           exchange_time"
                                                  ", quote_time) "
                                                  "  VALUES ";

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
    struct PSQLTable<NamesOfTradeAndQuoteTables> {
        static constexpr char sc_colsDefinition[] = "( ric VARCHAR(15)"
                                                    ", reuters_date DATE"
                                                    ", reuters_table_name VARCHAR(23)"
                                                    ", PRIMARY KEY (ric, reuters_date) ) ";

        static const char* name;

        enum VAL_IDX : int {
            RIC = 0,
            REUT_DATE,
            REUT_TABLE_NAME,

            NUM_FIELDS
        };
    };

    //----------------------------------------------------------------------------------------------------------

    template <>
    struct PSQLTable<DETradingRecords> {
        static constexpr char sc_colsDefinition[] = " (session_id VARCHAR(50)"
                                                    ", real_time TIMESTAMP"
                                                    ", execution_time TIMESTAMP"
                                                    ", symbol VARCHAR(15)"
                                                    ", price REAL"
                                                    ", size INTEGER"
                                                    ", trader_id_1 VARCHAR(40)"
                                                    ", trader_id_2 VARCHAR(40)"
                                                    ", order_id_1 VARCHAR(40)"
                                                    ", order_id_2 VARCHAR(40)"
                                                    ", order_type_1 VARCHAR(2)"
                                                    ", order_type_2 VARCHAR(2)"
                                                    ", time_1 TIMESTAMP"
                                                    ", time_2 TIMESTAMP"
                                                    ", decision CHAR"
                                                    ", destination VARCHAR(10)"
                                                    ")";

        static const char* name;
    };

    //----------------------------------------------------------------------------------------------------------

    template <>
    struct PSQLTable<BCTradingRecords> {
        static constexpr char sc_colsDefinition[] = "( real_time TIMESTAMP"
                                                    ", execute_time TIMESTAMP"
                                                    ", symbol VARCHAR(15)"
                                                    ", price REAL"
                                                    ", size INTEGER"

                                                    ", trader_id_1 UUID" // UUID: Map to traders.id
                                                    ", trader_id_2 UUID" // ditto
                                                    ", order_id_1 VARCHAR(40)"
                                                    ", order_id_2 VARCHAR(40)"
                                                    ", order_type_1 VARCHAR(2)"

                                                    ", order_type_2 VARCHAR(2)"
                                                    ", time1 TIMESTAMP"
                                                    ", time2 TIMESTAMP"
                                                    ", decision CHAR"
                                                    ", destination VARCHAR(10)"

                                                    ",  CONSTRAINT trading_records_pkey PRIMARY KEY (order_id_1, order_id_2)\
                                                    \
                                                 ,  CONSTRAINT trading_records_fkey_1 FOREIGN KEY (trader_id_1)\
                                                        REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                 ,  CONSTRAINT trading_records_fkey_2 FOREIGN KEY (trader_id_2)\
                                                        REFERENCES PUBLIC.traders (id) MATCH SIMPLE\
                                                        ON UPDATE NO ACTION ON DELETE NO ACTION\
                                                )";

        static constexpr char sc_recordFormat[] = "( real_time, execute_time, symbol, price, size"
                                                  ", trader_id_1, trader_id_2, order_id_1, order_id_2, order_type_1"
                                                  ", order_type_2, time1, time2, decision, destination"
                                                  ") VALUES ";

        static const char* name;

        enum RCD_VAL_IDX : int {
            REAL_TIME = 0,
            EXEC_TIME,
            SYMBOL,
            PRICE,
            SIZE,

            TRD_ID_1,
            TRD_ID_2,
            ODR_ID_1,
            ODR_ID_2,
            ODR_TY_1,

            ODR_TY_2,
            TIME_1,
            TIME_2,
            DECISION,
            DEST,

            NUM_FIELDS
        };
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

        static constexpr char sc_recordFormat[] = "( id, buying_power, holding_balance, borrowed_balance, total_pl"
                                                  ", total_shares"
                                                  ") VALUES ";

        static const char* name;

        enum VAL_IDX : int {
            ID,
            BP,
            HB,
            BB,
            TOTAL_PL,

            TOTAL_SH,

            NUM_FIELDS
        };
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

        static constexpr char sc_recordFormat[] = "( id, symbol, borrowed_balance, pl, long_price"
                                                  ", short_price, long_shares, short_shares"
                                                  ") VALUES ";

        static const char* name;

        enum VAL_IDX : int {
            ID,
            SYMBOL,
            BB,
            PL,
            LPRICE,

            SPRICE,
            LSHARES,
            SSHARES,

            NUM_FIELDS
        };
    };

} // database
} // shift