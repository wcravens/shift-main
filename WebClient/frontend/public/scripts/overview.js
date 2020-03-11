/*
 * rceive overview data and display
 */

$(document).ready(function () {
    $("#" + localStorage.getItem("cur_symbol")).toggleClass("row-active");
      var conn = new ab.Session("ws://" + php_server_ip + ":8080",
        function () {
            conn.subscribe('bestPrice', function (topic, data) {
                var bestPriceCells = document.getElementById(data.data.symbol).cells;
                if (bestPriceCells[1].innerHTML < numFloat(data.data.lastPrice) && bestPriceCells[1].innerHTML != "")
                    bestPriceCells[1].className = "rise bold";
                else if (bestPriceCells[1].innerHTML > numFloat(data.data.lastPrice))
                    bestPriceCells[1].className = "drop bold";
                else
                    bestPriceCells[1].className = "";
                bestPriceCells[1].innerHTML = numFloat(data.data.lastPrice);
                bestPriceCells[2].className = "rborder";
                bestPriceCells[3].innerHTML = numInt(data.data.best_bid_size);
                bestPriceCells[3].className = "notimpcol";
                if (bestPriceCells[4].innerHTML < numFloat(data.data.best_bid_price) && bestPriceCells[4].innerHTML != "")
                    bestPriceCells[4].className = "rise bold";
                else if (bestPriceCells[4].innerHTML > numFloat(data.data.best_bid_price))
                    bestPriceCells[4].className = "drop bold";
                else
                    bestPriceCells[4].className = "";
                bestPriceCells[4].innerHTML = numFloat(data.data.best_bid_price);
                if (bestPriceCells[5].innerHTML < numFloat(data.data.best_ask_price) && bestPriceCells[5].innerHTML != "")
                    bestPriceCells[5].className = "rise bold";
                else if (bestPriceCells[5].innerHTML > numFloat(data.data.best_ask_price))
                    bestPriceCells[5].className = "drop bold";
                else
                    bestPriceCells[5].className = "";
                bestPriceCells[5].innerHTML = numFloat(data.data.best_ask_price);
                bestPriceCells[6].innerHTML = numInt(data.data.best_ask_size);
                bestPriceCells[6].className = "notimpcol";
            });
        },
        function () {
            console.warn('WebSocket connection closed');
        },
        { 'skipSubprotocolCheck': true }
    );

    jQuery.fn.single_double_click = function (single_click_callback, double_click_callback, timeout) {
        return this.each(function () {
            var clicks = 0, self = this;
            jQuery(this).click(function (event) {
                clicks++;
                if (clicks == 1) {
                    setTimeout(function () {
                        if (clicks == 1) {
                            single_click_callback.call(self, event);
                        } else {
                            double_click_callback.call(self, event);
                        }
                        clicks = 0;
                    }, timeout || 200);
                }
            });
        });
    }

    $("#stock_list tr").single_double_click(function () {
        if ((this.id == "") || (this.id == localStorage.getItem("cur_symbol")))
            return;
        // window.location = "/overview.php?symbol=" + this.id;
        refreshSymbol("cur_symbol", this.id);
    }, function () {
        if (this.id == "")
            return;
        refreshSymbol("cur_symbol", this.id);
        window.location = "/orderbook.php?symbol=" + this.id;
    })

    function isElementInViewport(el) {
        //special bonus for those using jQuery
        if (typeof jQuery === "function" && el instanceof jQuery) {
            el = el[0];
        };
        var rect = el.getBoundingClientRect();
        return (
            rect.top >= 0 &&
            // make the navbar out of the way
            rect.top >= document.getElementById("myNavbar").offsetHeight &&
            rect.left >= 0 &&
            rect.bottom <= (window.innerHeight || document.documentElement.clientHeight) && /*or $(window).height() */
            rect.right <= (window.innerWidth || document.documentElement.clientWidth) /*or $(window).width() */
        );
    }

    // setTimeout(function(){ console.log(document.getElementById("myNavbar").offsetHeight); }, 30);
    $("#lock-check").attr("disabled", true);
    $("#lock-slider").css('background-color', '#777');
    $("#lock-ticker").toggleClass("disabled");

    $(function () {
        $(window).bind('refreshSymbolEvent storage', function (e) {
            indexCur = php_stockList_json.indexOf(localStorage.getItem("cur_symbol"));
            indexLas = php_stockList_json.indexOf(e.originalEvent.oldValue);
            $("#" + localStorage.getItem("cur_symbol")).toggleClass("row-active");
            if (!isElementInViewport(document.getElementById(localStorage.getItem("cur_symbol")))) {
                if (indexCur > indexLas)
                    document.getElementById(localStorage.getItem("cur_symbol")).scrollIntoView(false); //align bottom
                else {
                    document.getElementById(localStorage.getItem("cur_symbol")).scrollIntoView(true); //align top
                    window.scrollBy(0, -document.getElementById("myNavbar").offsetHeight); //scroll upwards by the size of myNavbar
                }
            }
            $("#" + e.originalEvent.oldValue).toggleClass("row-active");
        });
    });
});
