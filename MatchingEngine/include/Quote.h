#pragma once

#include <string>

class Quote {

private:
    std::string stockname;
    std::string trader_id;
    std::string order_id;
    long mili;
    double price;
    int size;
    std::string time;
    char ordertype;
    std::string destination;
    //(limit_buy,  limit_sell,  market_buy,  market_sell,  cancel_bid,  cancel_ask)

public:
    friend class Stock;
    //for the server to receive
    Quote(std::string stockname1, std::string trader_id1, std::string order_id1, double price1, int size1, std::string time1, char ordertype1);
    // for the client to send
    Quote(std::string stockname1, std::string trader_id1, std::string order_id1, double price1, int size1, char ordertype1);

    /**
	 * @brief Constructor for the Quote class.
	 * @param stockname, price, size, destination, time
	 * @return None.
	 */
    Quote(std::string stockname1, double price1, int size1, std::string destination1, std::string time1)
    {
        stockname = stockname1;
        trader_id = "TR";
        order_id = "Thomson";
        price = price1;
        size = size1;
        time = time1;
        ordertype = '1';
        destination = destination1;
    }

    Quote(void);
    Quote(const Quote& newquote);

    ~Quote(void);

    void operator=(const Quote& newquote);
    void setstockname(std::string name1);
    std::string getstockname();

    void settrader_id(std::string trader_id1);
    std::string gettrader_id();

    void setorder_id(std::string order_id1);
    std::string getorder_id();

    /**
	 * @brief Setter for mili field of current Quote object.
	 * @param mili to be set.
	 * @return None.
	 */
    void setmili(const long& _mili)
    {
        mili = _mili;
    }

    /**
	 * @brief Getter for mili field of current Quote object.
	 * @param None.
	 * @return mili of the current Quote object.
	 */
    long getmili()
    {
        return mili;
    }

    void setprice(double price1);
    double getprice();

    void setsize(int size1);
    int getsize();

    void settime(std::string time1);
    std::string gettime();

    void setordertype(char ordertype1);
    char getordertype();

    /**
	 * @brief Setter for destination field of current Quote object.
	 * @param destination to be set.
	 * @return None.
	 */
    void setdestination(std::string destination1) { destination = destination1; }

    /**
	 * @brief Getter for destination field of current Quote object.
	 * @param None.
	 * @return destination of the current Quote object.
	 */
    std::string getdestination() { return destination; }
};
