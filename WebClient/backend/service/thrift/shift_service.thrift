namespace php client
service SHIFTService{
    // map<string, string> hello(),

    void submitOrder(1:string username, 2:string orderType, 3:string orderSymbol, 4:i32 orderSize, 5:double orderPrice, 6:string orderID),

    void webClientSendUsername(1:string username),
    
    string webUserLoginV2(1:string username, 2:string password),
    void webUserLogin(1:string username),

    string getAllTraders()
    string getThisLeaderboard(1:string startDate = "", 2:string endDate = "")
}

