/*
 * rceive portfolio data and display
 */

function createCloseForm(symbol, size, mysubmit) {
    var f = document.createElement("form");
    f.setAttribute('class', 'form-inline');
    f.setAttribute('method', 'post');
    f.setAttribute('action', "./utility/sendorder.php");
    f.setAttribute("style", "padding-bottom: 0px; margin-bottom: 0px;");

    var i0 = document.createElement("input");
    i0.setAttribute('type', "hidden");
    i0.setAttribute('name', "symbol");
    i0.setAttribute('value', symbol);

    var i1 = document.createElement("input");
    i1.setAttribute('type', "hidden");
    i1.setAttribute('name', "price");
    i1.setAttribute('value', 0);

    var i2 = document.createElement("input");
    i2.setAttribute('type', "hidden");
    i2.setAttribute('name', "size");
    i2.setAttribute('value', size);

    var i3 = document.createElement("input");
    i3.setAttribute('type', "hidden");
    i3.setAttribute('name', "mysubmit");
    i3.setAttribute('value', mysubmit);

    var i4 = document.createElement("button");
    i4.setAttribute('type', "submit");
    i4.setAttribute('class', "btn btn-default btn-xs alignright notimpcol");
    i4.innerHTML = "Close Position";

    var i5 = document.createElement("input");
    i5.setAttribute('type', "hidden");
    i5.setAttribute('name', "redirect_url");
    i5.setAttribute('value', "/myportfolio.php");


    f.appendChild(i0);
    f.appendChild(i1);
    f.appendChild(i2);
    f.appendChild(i3);
    f.appendChild(i4);
    f.appendChild(i5);

    return f;
}

function createCancelOrderForm(orderType, symbol, size, price, orderId) {
    var f = document.createElement("form");
    f.setAttribute('class', 'form-inline alignright notwrap');
    f.setAttribute('method', 'post');
    f.setAttribute('action', "./utility/cancelorder.php");
    f.setAttribute("style", "padding-bottom: 0px; margin-bottom: 0px; display: inline;");

    var i0 = document.createElement("input");
    i0.setAttribute('type', "hidden");
    i0.setAttribute('name', "orderId");
    i0.setAttribute('value', orderId);

    var i1 = document.createElement("input");
    i1.setAttribute('type', "hidden");
    i1.setAttribute('name', "orderType");
    i1.setAttribute('value', orderType);

    var i2 = document.createElement("input");
    i2.setAttribute('type', "hidden");
    i2.setAttribute('name', "symbol");
    i2.setAttribute('value', symbol);

    var i3 = document.createElement("input");
    i3.setAttribute('id', "input_size" + orderId);
    i3.setAttribute('type', "number");
    i3.setAttribute('class', "input-xs notimpcol");
    i3.setAttribute('name', "size");
    i3.setAttribute('min', 1);
    i3.setAttribute('max', size);
    i3.setAttribute('style', "width: 4em");
    i3.setAttribute('value', size);
    i3.setAttribute('placeholder', "Size");
    i3.setAttribute('onchange', "sizeOnchangeFunc(orderId)");

    var i4 = document.createElement("input");
    i4.setAttribute('type', "submit");
    i4.setAttribute('class', "btn btn-default btn-xs notimpcol");
    i4.setAttribute('style', "width: 90px");
    i4.setAttribute('name', "submit");
    i4.setAttribute('value', "Cancel Order");

    var i5 = document.createElement("input");
    i5.setAttribute('type', "hidden");
    i5.setAttribute('name', "redirect_url");
    i5.setAttribute('value', "/myportfolio.php");

    var i6 = document.createElement("input");
    i6.setAttribute('type', "hidden");
    i6.setAttribute('name', "price");
    i6.setAttribute('value', price);

    var i7 = document.createElement("input");
    i7.setAttribute('id', "ori_size" + orderId);
    i7.setAttribute('type', "hidden");
    i7.setAttribute('value', size);

    f.appendChild(i0);
    f.appendChild(i1);
    f.appendChild(i2);
    f.appendChild(i3);
    f.appendChild(i4);
    f.appendChild(i5);
    f.appendChild(i6);
    f.appendChild(i7);

    return f;
}

// Set size back to original if user types in a larger one.
function sizeOnchangeFunc(orderId) {
    var inputSize = document.getElementById('input_size' + orderId.value);
    var oriSize = parseInt(document.getElementById('ori_size' + orderId.value).value);
    var floatrx = new RegExp(/^[+-]?\d+(\.\d+)?$/);
    if (inputSize.value <= 0) {
        inputSize.value = 1;
    } else {
        if (floatrx.test(inputSize.value)) {
            inputSize.value = Math.floor(inputSize.value);
        }
        if (inputSize.value > oriSize) {
            inputSize.value = oriSize;
        }
    }
}

$(document).ready(function () {
    let tableCells = 8; // cell count;
    var conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            conn.subscribe('portfolioSummary_' + php_username, function (topic, data) {
                document.getElementById("buying_power").innerHTML = numFloat(data.data.totalBP);
                document.getElementById("total_shares").innerHTML = numInt(data.data.totalShares);
                document.getElementById("total_shares").className = "text-right notimpcol";
                if (parseFloat(data.data.portfolioUnrealizedPL) > 0) {
                    document.getElementById("unrealized_pl").className = "text-right rise";
                } else if (parseFloat(data.data.portfolioUnrealizedPL) < 0) {
                    document.getElementById("unrealized_pl").className = "text-right drop";
                } else {
                    document.getElementById("unrealized_pl").className = "text-right";
                }
                if (parseFloat(data.data.totalPL) > 0) {
                    document.getElementById("total_pl").className = "text-right rise";
                    document.getElementById("earnings").className = "text-right rise";
                } else if (parseFloat(data.data.totalPL) < 0) {
                    document.getElementById("total_pl").className = "text-right drop";
                    document.getElementById("earnings").className = "text-right drop";
                } else {
                    document.getElementById("total_pl").className = "text-right";
                    document.getElementById("earnings").className = "text-right";
                }
                document.getElementById("unrealized_pl").innerHTML = numFloat(data.data.portfolioUnrealizedPL);
                document.getElementById("total_pl").innerHTML = numFloat(data.data.totalPL);
                document.getElementById("earnings").innerHTML = numFloat(parseFloat(data.data.earnings) * 100) + "%";

            });

            conn.subscribe('portfolio_' + php_username, function (topic, data) {
                var portfolio_table = document.getElementById("portfolio");
                if (data.data.length != 0 && document.getElementById("emptyPosition"))
                    portfolio_table.deleteRow(1);

                for (var i = 0; i < data.data.length; i++) {
                    var index = i + 1;
                    if (portfolio_table.rows[index]) {
                        portfolio_table.rows[index].cells[6].removeChild(portfolio_table.rows[index].cells[6].childNodes[0]);
                    } else {
                        var row = portfolio_table.insertRow(index);
                        var cell0 = row.insertCell(0);
                        cell0.className = "bold";
                        var cell1 = row.insertCell(1);
                        cell1.className = "text-right";
                        var cell2 = row.insertCell(2);
                        var cell3 = row.insertCell(3);
                        var cell4 = row.insertCell(4);
                        var cell5 = row.insertCell(5);
                        var cell6 = row.insertCell(6);
                    }
                    if (parseFloat(data.data[i].unrealizedPL) > 0) {
                        portfolio_table.rows[index].cells[4].className = "text-right rise";
                    } else if (parseFloat(data.data[i].unrealizedPL) < 0) {
                        portfolio_table.rows[index].cells[4].className = "text-right drop";
                    } else {
                        portfolio_table.rows[index].cells[4].className = "text-right";
                    }
                    if (parseFloat(data.data[i].pl) > 0) {
                        portfolio_table.rows[index].cells[5].className = "text-left rise";
                    } else if (parseFloat(data.data[i].pl) < 0) {
                        portfolio_table.rows[index].cells[5].className = "text-left drop";
                    } else {
                        portfolio_table.rows[index].cells[5].className = "text-left";
                    }
                    portfolio_table.rows[index].cells[0].innerHTML = data.data[i].symbol;
                    portfolio_table.rows[index].cells[1].innerHTML = numInt(data.data[i].shares);
                    portfolio_table.rows[index].cells[2].innerHTML = numFloat(data.data[i].price);
                    portfolio_table.rows[index].cells[2].className = "text-right notimpcol";
                    portfolio_table.rows[index].cells[3].innerHTML = numFloat(data.data[i].closePrice);
                    portfolio_table.rows[index].cells[3].className = "text-right notimpcol";
                    portfolio_table.rows[index].cells[4].innerHTML = numFloat(data.data[i].unrealizedPL);
                    portfolio_table.rows[index].cells[5].innerHTML = "(" + numFloat(data.data[i].pl) + ")";
                    portfolio_table.rows[index].cells[6].className = "text-right notimpcol";
                    var closeform = createCloseForm(data.data[i].symbol, Math.abs(data.data[i].shares / 100), data.data[i].shares > 0 ? "Market Sell" : "Market Buy");
                    // closeform.elements.item(4).style = "width: 90px";
                    if (data.data[i].shares == "0") {
                        portfolio_table.rows[index].className = "row_disabled";
                        closeform.elements.item(4).className = "btn btn-default btn-xs alignright disabled notimpcol";
                        closeform.elements.item(4).disabled = true;
                    } else
                        portfolio_table.rows[index].className = "";
                    portfolio_table.rows[index].cells[6].appendChild(closeform);
                }

                if (!portfolio_table.rows[1]) {
                    var row = portfolio_table.insertRow(1);
                    row.id = "emptyPosition";
                    cell = row.insertCell(0);
                    cell.colSpan = tableCells;
                    cell.innerHTML = "You don't have any positions yet.";
                    cell.className = "text-center";
                }
            });

            conn.subscribe('waitingList_' + php_username, function (topic, data) {
                var stock_list_table = document.getElementById("stock_list");
                if (data.data.length != 0 && document.getElementById("emptyWaitingList"))
                    stock_list_table.deleteRow(1);

                for (var i = 0; i < data.data.length; i++) {
                    var index = i + 1;
                    var cNum = 0;
                    let row;
                    if (stock_list_table.rows[index]) {
                        row = stock_list_table.rows[index];
                        row.cells[6].removeChild(row.cells[cNum].childNodes[0]);
                    } else {
                        row = stock_list_table.insertRow(index);
                        for (var j = 0; j < tableCells; j++) {
                            row.insertCell(j);
                        }
                        row.cells[0].className = "bold"; // symbol
                        row.cells[1].className = "text-right"; // order type
                        row.cells[2].className = "text-right"; // price
                        row.cells[3].className = "text-right"; // order size
                        row.cells[6].className = " notimpcol "; // cancel order form
                        row.cells[6].setAttribute("nowrap", "nowrap");
                    }
                    row.cells[cNum].innerHTML = data.data[i].symbol;
                    cNum++; // order type
                    row.cells[cNum].innerHTML = data.data[i].orderType;
                    cNum++; // price
                    row.cells[cNum].innerHTML = numFloat(data.data[i].price);
                    cNum++; // order size
                    row.cells[cNum].innerHTML = numInt(data.data[i].size);
                    cNum++; // empty column
                    cNum++; // order id
                    row.cells[cNum].innerHTML = data.data[i].orderId;
                    row.cells[cNum].className = " notimpcol2";
                    cNum++; // status
                    row.cells[cNum].innerHTML = data.data[i].status;
                    row.cells[cNum].className += " notimpcol2";
                    cNum++; // cancle order form
                    row.cells[cNum].appendChild(
                        createCancelOrderForm(
                            // If the order was "Limit Buy" or "Market Buy", we create a CALCEL_BID.
                            //   Otherwise if it was "Limit Sell" or "Market Sell", we create a CANCEL_ASK.
                            //   orderType doesn't have other possible value in this case.
                            (data.data[i].orderType == "Limit Buy" || data.data[i].orderType == "Market Buy")
                                ? "5"
                                : "6",
                            data.data[i].symbol,
                            data.data[i].size,
                            data.data[i].price,
                            data.data[i].orderId));
                }

                var len = stock_list_table.rows.length;
                for (var i = data.data.length; i < len - 1; i++) {
                    stock_list_table.deleteRow(-1);
                }

                if (!stock_list_table.rows[1]) {
                    var row = stock_list_table.insertRow(1);
                    row.id = "emptyWaitingList";
                    cell = row.insertCell(0);
                    cell.colSpan = tableCells;
                    cell.innerHTML = "Your waiting list is empty.";
                    cell.className = "text-center";
                }
            });
        },
        function () {
            console.warn('WebSocket connection closed');
        },
        { 'skipSubprotocolCheck': true }
    );
});