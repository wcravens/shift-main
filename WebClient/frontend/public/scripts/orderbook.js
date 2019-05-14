/*
 * rceive orderbook data and display
 */

//  insert price on click on orderbook rows
function insertPrice() {
    document.getElementById('price_input').value = this.cells[0].innerHTML;
    document.getElementById('quantity_input').value = 1;
}

$(document).ready(function () {
    var conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            conn.subscribe('orderBook_' + localStorage.getItem("cur_symbol"), function (topic, data) {
                var global_bid_table = document.getElementById("global_bid_table");
                var global_ask_table = document.getElementById("global_ask_table");
                var local_bid_table = document.getElementById("local_bid_table");
                var local_ask_table = document.getElementById("local_ask_table");
                var table_to_update = null;

                for (var i = 0; i < data.data.length; i++) {
                    _select = document.getElementById('symbol');
                    var index = i + 1;

                    if (data.data[i].bookType == 'A') {
                        if (data.data[i].size == 0) {
                            var len = global_ask_table.rows.length;
                            for (var i = 1; i < len; i++) {
                                global_ask_table.deleteRow(-1);
                            }
                        } else {
                            if (global_ask_table.rows[index]) {
                                global_ask_table.rows[index].cells[0].innerHTML = numFloat(data.data[i].price);
                                global_ask_table.rows[index].cells[1].innerHTML = numInt(data.data[i].size);
                                // ------------wipe out destination
                            } else {
                                var row = global_ask_table.insertRow(index);
                                row.className = "notfirst";
                                row.align = "right";
                                row.onclick = insertPrice;
                                var cell0 = row.insertCell(0);
                                var cell1 = row.insertCell(1);
                                // add more space before destination
                                row.insertCell(2);
                                // ------------wipe out destination
                                cell0.innerHTML = numFloat(data.data[i].price);
                                cell1.innerHTML = numInt(data.data[i].size);
                            }
                        }
                        table_to_update = global_ask_table;
                    } else if (data.data[i].bookType == 'B') {
                        if (data.data[i].size == 0) {
                            var len = global_bid_table.rows.length;
                            for (var i = 1; i < len; i++) {
                                global_bid_table.deleteRow(-1);
                            }
                        } else {
                            if (global_bid_table.rows[index]) {
                                global_bid_table.rows[index].cells[0].innerHTML = numFloat(data.data[i].price);
                                global_bid_table.rows[index].cells[1].innerHTML = numInt(data.data[i].size);
                                // ------------wipe out destination
                            } else {
                                var row = global_bid_table.insertRow(i + 1);
                                row.className = "notfirst";
                                row.align = "right";
                                row.onclick = insertPrice;
                                var cell0 = row.insertCell(0);
                                var cell1 = row.insertCell(1);
                                // add more space before destination
                                row.insertCell(2);
                                // ------------wipe out destination
                                cell0.innerHTML = numFloat(data.data[i].price);
                                cell1.innerHTML = numInt(data.data[i].size);
                            }
                        }
                        table_to_update = global_bid_table;
                    } else if (data.data[i].bookType == 'a') {
                        if (data.data[i].size == 0) {
                            var len = local_ask_table.rows.length;
                            for (var i = 1; i < len; i++) {
                                local_ask_table.deleteRow(-1);
                            }
                        } else {
                            if (local_ask_table.rows[index]) {
                                local_ask_table.rows[index].cells[0].innerHTML = numFloat(data.data[i].price);
                                local_ask_table.rows[index].cells[1].innerHTML = numInt(data.data[i].size);
                            } else {
                                var row = local_ask_table.insertRow(index);
                                row.className = "notfirst";
                                row.align = "right";
                                row.onclick = insertPrice;
                                var cell0 = row.insertCell(0);
                                var cell1 = row.insertCell(1);
                                // add more space before destination
                                row.insertCell(2);
                                // ------------wipe out destination
                                cell0.innerHTML = numFloat(data.data[i].price);
                                cell1.innerHTML = numInt(data.data[i].size);
                            }
                        }
                        table_to_update = local_ask_table;
                    } else if (data.data[i].bookType == 'b') {
                        if (data.data[i].size == 0) {
                            var len = local_bid_table.rows.length;
                            for (var i = 1; i < len; i++) {
                                local_bid_table.deleteRow(-1);
                            }
                        } else {
                            if (local_bid_table.rows[index]) {
                                local_bid_table.rows[index].cells[0].innerHTML = numFloat(data.data[i].price);
                                local_bid_table.rows[index].cells[1].innerHTML = numInt(data.data[i].size);
                            } else {
                                var row = local_bid_table.insertRow(i + 1);
                                row.className = "notfirst";
                                row.align = "right";
                                row.onclick = insertPrice;
                                var cell0 = row.insertCell(0);
                                var cell1 = row.insertCell(1);
                                // add more space before destination
                                row.insertCell(2);
                                // ------------wipe out destination
                                cell0.innerHTML = numFloat(data.data[i].price);
                                cell1.innerHTML = numInt(data.data[i].size);
                            }
                        }
                        table_to_update = local_bid_table;
                    }
                }
                if (table_to_update) {
                    var len = table_to_update.rows.length;
                    for (var i = data.data.length; i < len - 1; i++) {
                        table_to_update.deleteRow(-1);
                    }
                }
            });
        },
        function () {
            console.warn('WebSocket connection closed');
        },
        { 'skipSubprotocolCheck': true }
    );

    $(window).bind('refreshSymbolEvent storage', function (e) {
        if (e.type == "storage")
            if (!ticker_locked)
                return;
        // $("#"+localStorage.getItem("cur_symbol")).toggleClass("row-active");
        // $("#"+e.originalEvent.oldValue).toggleClass("row-active");
        window.location.replace("/orderbook.php");
    });

});