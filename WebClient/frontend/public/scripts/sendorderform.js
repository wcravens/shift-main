// default cur-symbol for first start up
if (localStorage.getItem('cur_symbol') === null) {
    localStorage.setItem('cur_symbol', php_stockList_json[0]);
}

// functions for sendorder form
function decimalPlaces(num) {
    var match = ('' + num).match(/(?:\.(\d+))?(?:[eE]([+-]?\d+))?$/);
    if (!match) { return 0; }
    return Math.max(
        0,
        // Number of digits right of decimal point.
        (match[1] ? match[1].length : 0)
        // Adjust for scientific notation.
        - (match[2] ? +match[2] : 0));
}

function checkBothFilled() {
    var price = document.getElementById('price_input').value;
    var size = document.getElementById('quantity_input').value;
    if (!price || !size) {
        alert('Please provide both price and size of your limit order.');
        return false;
    }
    return true;
}

function checkPrice() {
    var price = document.getElementById('price_input').value;
    if (isNaN(price) || price < 0) {
        alert('\"Price\" should be a positive number.');
        return false;
    }
    if (decimalPlaces(price) > 2) {
        alert('\"Price\" should be a positive number with at most two decimal places.');
        return false;
    }
    return true;
}

function checkSize() {
    var size = document.getElementById('quantity_input').value;
    if (!size) {
        alert('Please provide the size of your market order.');
        return false;
    }
    if (isNaN(size) || decimalPlaces(size) > 0 || size <= 0) {
        alert('\"Size\" should be a positive whole number.');
        return false;
    }
    return true;
}

function quantityOnchange(){
    var quantity = document.getElementById("quantity_input");
    var floatrx = new RegExp(/^[+-]?\d+(\.\d+)?$/);
    if (quantity.value <= 0 ){
        quantity.value = 1;
    } else if (floatrx.test(quantity.value)){
        quantity.value = Math.floor(quantity.value);
    }
}

document.getElementById('limit_buy_btn').onclick = function () {
    if (!checkBothFilled())
        return false;

    if (checkPrice() && checkSize()) {
        document.getElementById('mysubmit').value = 'Limit Buy';
        document.getElementById('sendorder_form').submit();
    }
    return false;
};

document.getElementById('limit_sell_btn').onclick = function () {
    if (!checkBothFilled())
        return false;

    if (checkPrice() && checkSize()) {
        document.getElementById('mysubmit').value = 'Limit Sell';
        document.getElementById('sendorder_form').submit();
    }
    return false;
};


document.getElementById('market_buy_btn').onclick = function () {
    if (checkSize()) {
        document.getElementById('mysubmit').value = 'Market Buy';
        document.getElementById('sendorder_form').submit();
    }
    return false;
};

document.getElementById('market_sell_btn').onclick = function () {
    if (checkSize()) {
        document.getElementById('mysubmit').value = 'Market Sell';
        document.getElementById('sendorder_form').submit();
    }
    return false;
};

var symbol_select = document.getElementById('symbol');
symbol_select.onchange = function () {
    refreshSymbol("cur_symbol", this.value);
}

// prepare the sendorder_form while the page is loaded
$(document).ready(function () {
    $("#" + localStorage.getItem("cur_symbol") + "_select").prop("selected", true);
    document.getElementById('sendorder_form').action="./utility/sendorder.php?symbol="+localStorage.getItem("cur_symbol");

    // update sendorder_form after refreshed the symbol
    $(window).bind('refreshSymbolEvent storage', function (e) {
        if (!ticker_locked)
            return;

        $("#symbol").val(localStorage.getItem("cur_symbol"));
        document.getElementById('sendorder_form').action = "./utility/sendorder.php?symbol=" + localStorage.getItem("cur_symbol");
    });
})