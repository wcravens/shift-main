/*
 * receive submitted orders data and display
 */

var orderTypeStrs = { "1": "Limit Buy", "2": "Limit Sell", "3": "Market Buy", "4": "Market Sell" };

$(document).ready(function () {
    var conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            conn.subscribe('submittedOrders_' + php_username, function (topic, data) {
                var stock_list_table = document.getElementById("stock_list");
                if (data.data.length != 0 && document.getElementById("emptyList"))
                    stock_list_table.deleteRow(1);
                        
                for (var i = 0; i < data.data.length; i++) {
                    var index = i + 1;                    
                    if (stock_list_table.rows[index]) {
                        stock_list_table.rows[index].cells[0].innerHTML = data.data[i].symbol;
                        stock_list_table.rows[index].cells[1].innerHTML = (data.data[i].orderType == "3" || data.data[i].orderType == "4") ? "" : numFloat(data.data[i].price);
                        stock_list_table.rows[index].cells[2].innerHTML = numInt(data.data[i].shares);
                        stock_list_table.rows[index].cells[4].innerHTML = orderTypeStrs[data.data[i].orderType];
                        stock_list_table.rows[index].cells[5].innerHTML = data.data[i].orderId;
                        stock_list_table.rows[index].cells[5].className += " notimpcol2";
                        stock_list_table.rows[index].cells[6].className += " notimpcol";
                        stock_list_table.rows[index].cells[7].className += " notimpcol";
                    } else {
                        var row = stock_list_table.insertRow(index);
                        var cell0 = row.insertCell(0);
                        cell0.className = "bold";
                        var cell1 = row.insertCell(1);
                        cell1.className = "text-right";
                        var cell2 = row.insertCell(2);
                        cell2.className = "text-right";
                        var cell3 = row.insertCell(3); // nothing
                        cell3.className = "text-right";
                        var cell4 = row.insertCell(4); // order type
                        var cell5 = row.insertCell(5); // order id
                        var cell6 = row.insertCell(6); // accepted flag
                        var cell7 = row.insertCell(7); // cancelled flag

                        cell0.innerHTML = data.data[i].symbol;
                        cell1.innerHTML = (data.data[i].orderType == "3" || data.data[i].orderType == "4") ? "" : numFloat(data.data[i].price);;
                        cell2.innerHTML = numInt(data.data[i].shares);
                        cell4.innerHTML = orderTypeStrs[data.data[i].orderType];
                        cell5.innerHTML = data.data[i].orderId;
                        cell5.className = " notimpcol2";
                        cell6.className = " notimpcol";
                        cell7.className = " notimpcol";
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
                    cell.colSpan = 8;
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