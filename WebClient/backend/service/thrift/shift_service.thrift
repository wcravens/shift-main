namespace php client
service SHIFTService{
    // map<string, string> hello(),

    // void submitOrder(1:string stockName, 2:string orderID, 3:double price, 4:i32 shareSize, 5:string orderType, 6:string username),
    void submitOrder(1:string username, 2:string orderType, 3:string orderSymbol, 4:i32 orderSize, 5:double orderPrice, 6:string orderID),

    void webClientSendUsername(1:string username),
    
    void webUserLogin(1:string username)

    // void startStrategy(1:string stockName, 2:double price, 3:i32 shareSize, 4:string orderType, 5:string username, 6:i32 interval)
}

    //map<string, string> hello(),
    ////need to convert 7:string to 7:char at server side
    //void  sendOrder(1:string stockName, 2:string traderId, 3:string orderId,
    //                4:double price, 5:i32 size, 6:string time, 7:string orderType ),

    //string getOrderBookJson(1:string stockName),

    //map<i32, CandleStickData> getCandleStickDataByStockName(1:string stockName),

    //list<string> getStockList(),

    //string get_best_to_json()
