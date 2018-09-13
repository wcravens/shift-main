/*
 * receive last price and display according to different rule
 */

function createsession() {
    conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            conn.subscribe('lastPriceView_' + localStorage.getItem("cur_symbol"), function (topic, data) {
                var lastPrice = document.getElementById("last_price");
                var rate = document.getElementById("rate");
                var diff = document.getElementById("diff");
                var arrow = document.getElementById("arrow");
                if (lastPrice.innerHTML < parseFloat(data.data.lastPrice) && lastPrice.innerHTML != "0.00") {
                    lastPrice.className = "bold rise";
                }
                else if (lastPrice.innerHTML > parseFloat(data.data.lastPrice)) {
                    lastPrice.className = "bold drop";
                }
                /* Four orderBooks at a time (0.5 seconds), only flash back to black when Global Ask comes*/
                else {
                    lastPrice.className = "boldlablelastprice";
                }

                if (parseFloat(data.data.rate) > 0) {
                    diff.className = "rise";
                    rate.className = "rise";
                    arrow.className = "glyphicon glyphicon-arrow-up glyphicon-bigger rise";
                }
                else if (parseFloat(data.data.rate) < 0) {
                    diff.className = "drop";
                    rate.className = "drop";
                    arrow.className = "glyphicon glyphicon-arrow-down glyphicon-bigger drop";
                }
                else {
                    diff.className = "";
                    rate.className = "";
                    arrow.className = "glyphicon glyphicon-stop glyphicon-smaller";
                }

                lastPrice.innerHTML = numFloat(data.data.lastPrice);
                diff.innerHTML = numFloat(Math.abs(data.data.diff));
                rate.innerHTML = "(" + numFloat(Math.abs(parseFloat(data.data.rate) * 100)) + "%)";
            });
        },
        function () {
            console.warn('WebSocket connection closed');
        },
        { 'skipSubprotocolCheck': true }
    );
    return conn;
}

$(document).ready(function () {
    var connection;
    connection = createsession();

    $(window).bind('refreshSymbolEvent storage', function (e) {
        if (!ticker_locked)
            return;

        connection.close();
        connection = createsession();
    });
});