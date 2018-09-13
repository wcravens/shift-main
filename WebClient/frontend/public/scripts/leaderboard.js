/*
 * rceive leaderboard data and display
 */

$(document).ready(function () {
    var conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            var leaderboard_table = document.getElementById("leaderboard");
            conn.subscribe('leaderboard', function (topic, data) {
                //console.log(data);
                for (var i = 0; i < data.data.length; i++) {
                    var index = i + 1;
                    if (leaderboard_table.rows[index]) {

                    } else {
                        var row = leaderboard_table.insertRow(index);
                        row.className = "clickable-row";
                        row.id = data.data[i].username;
                        var cell0 = row.insertCell(0);
                        cell0.className = "text-right bold";
                        row.insertCell(1);
                        var cell2 = row.insertCell(2);
                        var cell3 = row.insertCell(3);
                        cell3.className = "text-right notimpcol";
                        var cell4 = row.insertCell(4);
                        cell4.className = "text-right notimpcol";
                        var cell5 = row.insertCell(5);
                        cell5.className = "text-right notimpcol";
                        var cell6 = row.insertCell(6);
                        cell6.className = "text-right";
                        // cell6.className = "text-right";
                    }
                    if (parseFloat(data.data[i].totalPL) > 0) {
                        leaderboard_table.rows[index].cells[5].className = "text-right rise notimpcol";
                        leaderboard_table.rows[index].cells[6].className = "text-right rise";
                    } else if (parseFloat(data.data[i].totalPL) < 0) {
                        leaderboard_table.rows[index].cells[5].className = "text-right drop notimpcol";
                        leaderboard_table.rows[index].cells[6].className = "text-right drop";
                    } else {
                        leaderboard_table.rows[index].cells[5].className = "text-right notimpcol";
                        leaderboard_table.rows[index].cells[6].className = "text-right";
                    }
                    leaderboard_table.rows[index].cells[0].innerHTML = index;
                    leaderboard_table.rows[index].cells[2].innerHTML = data.data[i].username;
                    leaderboard_table.rows[index].cells[3].innerHTML = numFloat(data.data[i].totalBP);
                    leaderboard_table.rows[index].cells[4].innerHTML = numInt(data.data[i].totalShares);
                    leaderboard_table.rows[index].cells[5].innerHTML = numFloat(data.data[i].totalPL);
                    leaderboard_table.rows[index].cells[6].innerHTML = numFloat(parseFloat(data.data[i].earnings) * 100) + "%";
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