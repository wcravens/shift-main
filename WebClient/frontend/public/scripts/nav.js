/*
 * rceive nav data (PL) and display
 * support theme toggle
 */

var cur_symbol = "none";
var ticker_locked = true;

var themesheet = "";
var custthemecss = "";
if (JSON.parse(localStorage.getItem('dark-theme-enabled'))) {
    themesheet = $('<link rel="stylesheet" href="/static/bootstrap/css/bootstrap.cyborg.min.css">');
    custthemecss = $('<link rel="stylesheet" href="/style/theme_cyborg.css?version=' + php_shiftversion + '">');
    document.getElementById("changethemebtn").innerHTML = "wb_sunny";
} else {
    // themesheet = $('<link rel="stylesheet" href="/static/bootstrap/css/bootstrap.min.css">');
    themesheet = $('<link rel="stylesheet" href="/static/bootstrap/css/bootstrap.css">');
    custthemecss = $('<link rel="stylesheet" href="/style/theme_origin.css?version=' + php_shiftversion + '">');
    document.getElementById("changethemebtn").innerHTML = "brightness_3";
}

if (JSON.parse(localStorage.getItem('tickerlocked'))) {
} else {
    $('#lock-check').removeAttr('checked');
}

themesheet.appendTo('head');
custthemecss.appendTo('head');

$('.theme-link').click(function () {
    // var themeurl = "//bootswatch.com/slate/bootstrap.min.css";
    var themeurl = "//bootswatch.com/slate/bootstrap.css";
    themesheet.attr('href', themeurl);
});

function toggleDarkTheme() {
    if (JSON.parse(localStorage.getItem('dark-theme-enabled'))) {
        document.getElementById("changethemebtn").innerHTML = "brightness_3";
        localStorage.setItem('dark-theme-enabled', false);
        // themesheet.attr('href', "/static/bootstrap/css/bootstrap.min.css");
        themesheet.attr('href', "/static/bootstrap/css/bootstrap.css");
    } else {
        document.getElementById("changethemebtn").innerHTML = "wb_sunny";
        localStorage.setItem('dark-theme-enabled', true);
        themesheet.attr('href', "/static/bootstrap/css/bootstrap.cyborg.min.css");
    }

    myEvent = document.createEvent('Event');
    myEvent.initEvent('themechanged', true, true);
    document.dispatchEvent(myEvent);

    location.reload();
}

function togglelock() {
    // if ($('#lock-check').prop('checked'))
    //     localStorage.setItem('tickerlocked', true);
    // else 
    //     localStorage.setItem('tickerlocked', false);

    // change it from localstorage to a page based variable
    if ($('#lock-check').prop('checked'))
        ticker_locked = true;
    else
        ticker_locked = false;
}

$(document).ready(function () {
    // display hidden when this is ready
    $('body').css('display', 'block');

    if (!isTouchDevice()) {
        $('[data-toggle="popover"]').popover();
    }

    var conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            conn.subscribe('portfolioSummary_' + php_username, function (topic, data) {
                document.getElementById("BP").innerHTML = "BP: " + numFloat(data.data.totalBP) + " | P&L: " + numFloat(data.data.totalPL) + " (" + numFloat(parseFloat(data.data.earnings) * 100) + "%)";
            });
        },
        function () {
            console.warn('WebSocket connection closed');
        }, {
            'skipSubprotocolCheck': true
        }
    );

    $(window).bind('storage', function (e) {
        // location.reload();
        if (e.originalEvent.oldValue === "true" || e.originalEvent.oldValue === "false")
            location.reload();
    });

    // prevent dropdown window from disappearing after click (only for keep-open-on-click class)
    $(document).on('click', '.dropdown-menu', function (e) {
        if ($(this).hasClass('keep-open-on-click')) { e.stopPropagation(); }
    });
});