/*
 * js key binding
 */

// get the right next or last ticker index
index = 0;

function getNextSymbol() {
    index = 0;
    index = php_stockList_json.indexOf(localStorage.getItem("cur_symbol"));
    if (index >= 0 && index < php_stockList_json.length - 1)
        nextIndex = index + 1;
    else
        nextIndex = 0;
    return php_stockList_json[nextIndex];
}
function getPreSymbol() {
    index = 0;
    index = php_stockList_json.indexOf(localStorage.getItem("cur_symbol"));
    if (index == 0)
        preIndex = php_stockList_json.length - 1;
    else
        preIndex = index - 1;
    return php_stockList_json[preIndex];
}

function refreshSymbol(key, value) {
    myEvent = document.createEvent('Event');
    myEvent.initEvent('refreshSymbolEvent', true, true);
    myEvent.oldValue = localStorage.getItem('cur_symbol');

    localStorage.setItem(key, value);

    document.dispatchEvent(myEvent);
}

$(document).keydown(function (e) {
    switch (e.which) {
        case 37: // left
            refreshSymbol('cur_symbol', getPreSymbol());
            break;

        case 38: // up
            break;

        case 39: // right
            refreshSymbol('cur_symbol', getNextSymbol());
            break;

        case 40: // down
            break;

        case 81: // q
            window.location.replace("/overview.php");
            break;

        case 87: // w
            window.location.replace("/orderbook.php")
            break;

        case 69: // e
            window.location.replace("/myportfolio.php");
            break;

        case 82: // r
            window.location.replace("/leaderboard.php");
            break;

        case 80: // p
            document.getElementById("changethemebtn").click();
            break;

        case 65: // a
            document.getElementById("market_buy_btn").click();
            break;

        case 83: // s
            document.getElementById("market_sell_btn").click();
            break;

        case 68: // d
            document.getElementById("limit_buy_btn").click();
            break;

        case 70: // f
            document.getElementById("limit_sell_btn").click();
            break;

        case 84: // t
            document.getElementById("price_input").focus();
            break;

        case 71: // g
            document.getElementById("quantity_input").focus();
            break;

        default:
            // console.log(e.which);
            break;
    }
    // e.preventDefault(); // prevent the default action (scroll / move caret)
});