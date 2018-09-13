namespace php client
service SHIFTService{
    // map<string, string> hello(),

    // changed back becaus cancle order quote needs to have orderID as pramater
    //convert 5:string to 5:char at server side
    // void sendQuote(1:string stockName, 2:double price, 3:i32 shareSize, 4:string orderType, 5:string username),
    // void sendQuote(1:string stockName, 2:string orderID, 3:double price, 4:i32 shareSize, 5:string orderType, 6:string username),
    void submitOrder(1:string stockName, 2:string orderID, 3:double price, 4:i32 shareSize, 5:string orderType, 6:string username),

    void webClientSendUsername(1:string username),
    
    void webUserLogin(1:string username),

    // void startStrategy(1:string stockName, 2:string orderID, 3:double price, 4:i32 shareSize, 5:string orderType, 6:string username, 7:i32 interval)
    void startStrategy(1:string stockName, 2:double price, 3:i32 shareSize, 4:string orderType, 5:string username, 6:i32 interval)
}

    //map<string, string> hello(),
    ////need to convert 7:string to 7:char at server side
    //void  sendOrder(1:string stockName, 2:string traderId, 3:string orderId,
    //                4:double price, 5:i32 size, 6:string time, 7:string orderType ),

    //string getOrderBookJson(1:string stockName),

    //map<i32, CandleStickData> getCandleStickDataByStockName(1:string stockName),

    //list<string> getStockList(),

    //string get_best_to_json()
