namespace php client
service SHIFTService{
    // map<string, string> hello(),

    void submitOrder(1:string username, 2:string orderType, 3:string orderSymbol, 4:i32 orderSize, 5:double orderPrice, 6:string orderID),

    void webClientSendUsername(1:string username),
    
    string registerUser(1:string username, 2:string firstname, 3:string lastname, 4:string email, 5:string password),
    string webUserLoginV2(1:string username, 2:string password),
    void webUserLogin(1:string username),
    string is_login(1:string sessionid),
    string change_password(1:string cur_password, 2:string new_password, 3:string username),
    string get_user_by_username(1:string username),
    string getAllTraders(),
    string getThisLeaderboard(1:string startDate = "", 2:string endDate = ""),
    string getThisLeaderboardByDay(1:i32 contestDay)
}

