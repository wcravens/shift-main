/*
 * receive submitted orders data and display
 */

var orderTypeStrs = { "1": "Limit Buy", "2": "Limit Sell", "3": "Market Buy", "4": "Market Sell" };

$(document).ready(function () {
    var conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            conn.subscribe('submittedOrders_' + php_username, function (topic, data) {
                var stock_list_table = document.getElementById("stock_list");
                var tableCells = 8; // cell count
                if (data.data.length != 0 && document.getElementById("emptyList"))
                    stock_list_table.deleteRow(1);
                        
                for (var i = 0; i < data.data.length; i++) {
                    var index = i + 1;                    
                    var cNum = 0;
                    if (stock_list_table.rows[index]) {
                        stock_list_table.rows[index].cells[cNum].innerHTML = data.data[i].symbol;
                        cNum++; // order type
                        stock_list_table.rows[index].cells[cNum].innerHTML = orderTypeStrs[data.data[i].orderType];
                        cNum++; // price
                        stock_list_table.rows[index].cells[cNum].innerHTML = (data.data[i].orderType == "3" || data.data[i].orderType == "4") ? "" : numFloat(data.data[i].price);
                        cNum++; // order size
                        stock_list_table.rows[index].cells[cNum].innerHTML = numInt(data.data[i].shares);
                        cNum++; // empty column
                        cNum++; // order id
                        stock_list_table.rows[index].cells[cNum].innerHTML = data.data[i].orderId;
                        stock_list_table.rows[index].cells[cNum].className += " notimpcol2";
                        cNum++; // accepted flag
                        stock_list_table.rows[index].cells[cNum].className += " notimpcol";
                        cNum++; // cancelled flag
                        stock_list_table.rows[index].cells[cNum].className += " notimpcol";
                    } else {
                        var row = stock_list_table.insertRow(index);
                        for (var j = 0; j < tableCells; j++){
                            row.insertCell(j);
                        }
                        row.cells[cNum].innerHTML = data.data[i].symbol;
                        row.cells[cNum].className = "bold";
                        cNum++; // order type
                        row.cells[cNum].innerHTML = orderTypeStrs[data.data[i].orderType];
                        row.cells[cNum].className = "text-right";
                        cNum++; // price
                        row.cells[cNum].innerHTML = (data.data[i].orderType == "3" || data.data[i].orderType == "4") ? "" : numFloat(data.data[i].price);;
                        row.cells[cNum].className = "text-right";
                        cNum++; // order size
                        row.cells[cNum].innerHTML = numInt(data.data[i].shares);
                        row.cells[cNum].className = "text-right";
                        cNum++; // empty column
                        cNum++; // order id
                        row.cells[cNum].innerHTML = data.data[i].orderId;
                        row.cells[cNum].className = " notimpcol2";
                        cNum++; // accepted flag
                        row.cells[cNum].className = " notimpcol";
                        cNum++; // cancelled flag
                        row.cells[cNum].className = " notimpcol";
                    }
                }

                var len = stock_list_table.rows.length;
                for (var i = data.data.length; i < len - 1; i++) {
                    stock_list_table.deleteRow(-1);
                }

                if (!stock_list_table.rows[1]){
                    var row = stock_list_table.insertRow(1);
                    row.id = "emptyList";
                    cell = row.insertCell(0);
                    cell.colSpan = tableCells;
                    cell.innerHTML = "You haven't submitted any orders yet.";
                    cell.className = "text-center";
                }
            });
        },
        function () {
            console.warn('WebSocket connection closed');
        },
        { 'skipSubprotocolCheck': true }
    );
    $("#lock-check").attr("disabled", true);
    $("#lock-slider").css('background-color', '#777');
    $("#lock-ticker").toggleClass("disabled");
});