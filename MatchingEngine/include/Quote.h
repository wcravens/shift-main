#pragma once

#include <quickfix/FieldTypes.h>

#include <string>

class Quote {

private:
    std::string stockname;
    std::string trader_id;
    std::string order_id;
    long mili;
    double price;
    int size;
    char ordertype;
    std::string destination;
    FIX::UtcTimeStamp time;
    //(limit_buy,  limit_sell,  market_buy,  market_sell,  cancel_bid,  cancel_ask)

public:
    friend class Stock;
    //for the server to receive
    Quote(std::string stockname1, std::string trader_id1, std::string order_id1, double price1, int size1, char ordertype1, FIX::UtcTimeStamp utc);

    // for the client to send
    Quote(std::string stockname1, std::string trader_id1, std::string order_id1, double price1, int size1, char ordertype1);

    /**
	 * @brief Constructor for the Quote class.
	 * @param stockname, price, size, destination, time
	 * @return None.
	 */
    Quote(std::string stockname1, double price1, int size1, std::string destination1, FIX::UtcTimeStamp time);

    Quote(void);
    Quote(const Quote& newquote);

    ~Quote(void);

    void operator=(const Quote& newquote);

    void setstockname(std::string name1);
    std::string getstockname();

    std::string gettrader_id();

    std::string getorder_id();

    void setmili(long _mili);
    long getmili();

    void setprice(double price1);
    double getprice();

    void setsize(int size1);
    int getsize();

    FIX::UtcTimeStamp gettime();

    void setordertype(char ordertype1);
    char getordertype();

    void setdestination(std::string destination1);
    std::string getdestination();
};
