// check if stock symbol is still available
var index = -1;
index = php_stockList_json.indexOf(localStorage.getItem("cur_symbol"));
if (index == -1)
    localStorage.setItem("cur_symbol", php_stockList_json[0]);