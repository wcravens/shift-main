$(document).ready(function () {
    // highcharts dark theme settings
    HighchartsTheme = {
        colors: ['#2b908f', '#90ee7e', '#f45b5b', '#7798BF', '#aaeeee', '#ff0066', '#eeaaee',
            '#55BF3B', '#DF5353', '#7798BF', '#aaeeee'],
        chart: {
            backgroundColor: {
            },
            style: {
                fontFamily: '\'Unica One\', sans-serif'
            },
            plotBorderColor: '#606063'
        },
        title: {
            style: {
                color: '#E0E0E3',
                fontSize: '16px'
            }
        },
        subtitle: {
            style: {
                color: '#E0E0E3',
                textTransform: 'uppercase'
            }
        },
        xAxis: {
            gridLineColor: '#707073',
            labels: {
                style: {
                    color: '#E0E0E3'
                }
            },
            lineColor: '#707073',
            minorGridLineColor: '#505053',
            tickColor: '#707073',
            title: {
                style: {
                    color: '#A0A0A3'
                }
            }
        },
        yAxis: {
            gridLineColor: '#707073',
            labels: {
                style: {
                    color: '#E0E0E3'
                }
            },
            lineColor: '#707073',
            minorGridLineColor: '#505053',
            tickColor: '#707073',
            tickWidth: 1,
            title: {
                style: {
                    color: '#A0A0A3'
                }
            }
        },
        tooltip: {
            backgroundColor: 'rgba(0, 0, 0, 0.85)',
            style: {
                color: '#F0F0F0'
            }
        },
        plotOptions: {
            series: {
                dataLabels: {
                    color: '#B0B0B3'
                },
                marker: {
                    lineColor: '#333'
                }
            },
            boxplot: {
                fillColor: '#505053'
            },
            candlestick: {
                lineColor: 'white'
            },
            errorbar: {
                color: 'white'
            }
        },
        legend: {
            itemStyle: {
                color: '#E0E0E3'
            },
            itemHoverStyle: {
                color: '#FFF'
            },
            itemHiddenStyle: {
                color: '#606063'
            }
        },
        credits: {
            style: {
                color: '#666'
            }
        },
        labels: {
            style: {
                color: '#707073'
            }
        },

        drilldown: {
            activeAxisLabelStyle: {
                color: '#F0F0F3'
            },
            activeDataLabelStyle: {
                color: '#F0F0F3'
            }
        },

        navigation: {
            buttonOptions: {
                symbolStroke: '#DDDDDD',
                theme: {
                    fill: '#505053'
                }
            }
        },

        // scroll charts
        rangeSelector: {
            buttonTheme: {
                fill: '#505053',
                stroke: '#000000',
                style: {
                    color: '#CCC'
                },
                states: {
                    hover: {
                        fill: '#707073',
                        stroke: '#000000',
                        style: {
                            color: 'white'
                        }
                    },
                    select: {
                        fill: '#29292c',
                        stroke: '#000000',
                        style: {
                            color: 'white'
                        }
                    },
                    disabled: {
                        fill: '#000000',
                        stroke: '#000000',
                        style: {
                            color: 'grey'
                        }
                    }
                }
            },
            inputBoxBorderColor: '#505053',
            inputStyle: {
                backgroundColor: '#333',
                color: 'silver'
            },
            labelStyle: {
                color: 'silver'
            }
        },

        navigator: {
            handles: {
                backgroundColor: '#666',
                borderColor: '#AAA'
            },
            outlineColor: '#CCC',
            maskFill: 'rgba(255,255,255,0.1)',
            series: {
                color: '#CE7F00',
                lineColor: '#E59E2B'
            },
            xAxis: {
                gridLineColor: '#505053'
            }
        },

        scrollbar: {
            barBackgroundColor: '#808083',
            barBorderColor: '#808083',
            buttonArrowColor: '#CCC',
            buttonBackgroundColor: '#606063',
            buttonBorderColor: '#606063',
            rifleColor: '#FFF',
            trackBackgroundColor: '#404043',
            trackBorderColor: '#404043'
        },

        // special colors for some of the
        legendBackgroundColor: 'rgba(0, 0, 0, 0.5)',
        background2: '#505053',
        dataLabelsColor: '#B0B0B3',
        textColor: '#C0C0C0',
        contrastTextColor: '#F0F0F3',
        maskColor: 'rgba(255,255,255,0.3)'
    };

    Highcharts.setOptions({
        global: {
            useUTC: false,
        },
        lang: {
            rangeSelectorZoom: "Zoom:"
        }
    });

    // enable darktheme if localstorage was set to darktheme
    if (JSON.parse(localStorage.getItem('dark-theme-enabled')))
        Highcharts.setOptions(HighchartsTheme);

    // disable input box (to the right, date input)
    Highcharts.wrap(Highcharts.RangeSelector.prototype, 'drawInput', function (proceed, name) {
        proceed.call(this, name);
        this[name + 'DateBox'].on('click', function () { });
    });

    // set highchart options
    var highchartOptions = {
        chart: {
            renderTo: "candle_data_container"
        },
        scrollbar: {
            enabled: false
        },
        rangeSelector: {
            selected: 4,
            buttons: [{
                type: 'minute',
                count: 1,
                text: '1m',
                dataGrouping: {
                    forced: true,
                    units: [[
                        'second',
                        [1]
                    ]]
                }
            },
            {
                type: 'minute',
                count: 10,
                text: '10m',
                dataGrouping: {
                    forced: true,
                    units: [[
                        'second',
                        [5]
                    ]]
                }
            },
            {
                type: 'minute',
                count: 30,
                text: '30m',
                dataGrouping: {
                    forced: true,
                    units: [[
                        'second',
                        [15]
                    ]]
                }
            },
            {
                type: 'hour',
                count: 1,
                text: '1h',
                dataGrouping: {
                    forced: true,
                    units: [[
                        'second',
                        [30]
                    ]]
                }
            },
            {
                type: 'all',
                text: 'All',
                dataGrouping: {
                    PixelWidth: 100,
                    forced: true,
                    units: [[
                        'millisecond', // unit name
                        [1, 2, 5, 10, 20, 25, 50, 100, 200, 500] // allowed multiples
                    ], [
                        'second',
                        [1, 2, 5, 10, 15, 30, 45]
                    ], [
                        'minute',
                        [1, 2, 5, 10, 15, 30, 45]
                    ], [
                        'hour',
                        [1, 2, 3, 4, 6, 8, 12]
                    ], [
                        'day',
                        [1]
                    ], [
                        'week',
                        [1]
                    ], [
                        'month',
                        [1, 3, 6]
                    ], [
                        'year',
                        null
                    ]]
                }
            }]
        },

        title: {
            text: php_symbol_companyName_map_json[
                    localStorage.getItem("cur_symbol")] ?
                    (localStorage.getItem("cur_symbol")
                            + " ("
                            + php_symbol_companyName_map_json[localStorage
                                    .getItem("cur_symbol")]
                                    .replace("?", "\'")
                                    .replace("=", "\"")
                                    + ")")
                    : localStorage.getItem("cur_symbol"),
            style: {
                fontWeight: 'bold'
            }
        },

        // "exporting": { "enabled": true },
        "yAxis": [{
            "height": "100%",
            "top": "0%",
            "offset": 0,
            "id": "Pane-uze8060f6r",
            "type": "linear"
        }],

        "series": [{
            "name": localStorage.getItem("cur_symbol"),
            "id": localStorage.getItem("cur_symbol"),
            "yAxis": "Pane-uze8060f6r",
            "type": "candlestick",
            // "type": "area",
            "lineColor": "#9C9C9C",
            "upColor": "#51b749",
            "color": "#dc2127"
        }]
    };

    // refresh cur frequency on each time click on chart
    (function (H) {
        Highcharts.Chart.prototype.callbacks.push(function (chart) {
            H.addEvent(chart.container, 'click', function (e) {
                e = chart.pointer.normalize();
                curPeriod.attr({
                        text: '<span style="color: #6f6f6f; font-weight: bold;">'
                        + series.currentDataGrouping.count
                        + ' ' + series.currentDataGrouping.unitName
                        + (series.currentDataGrouping.count == 1 ? '' : 's')
                        });
            });
        });
    }(Highcharts));

    let chart = new Highcharts.StockChart(highchartOptions);
    var series = chart.series[0];

    chart.renderer.label('Freq:', 11, 80, 'callout')
        .css({
            color: '#6f6f6f'
        })
        .attr({
            padding: 5,
            r: 10,
            zIndex: 6
        })
        .add();

    // time zone name
    let timezone = jstz.determine();
    let tzname = timezone.name().replace(/^.*\//, "").replace("_", " ");
    tzname = 'Local Time Zone: ' + tzname;

    chart.renderer.label(tzname, 6, 326, 'callout')
        .css({
            color: '#6f6f6f'
        })
        .attr({
            padding: 5,
            r: 10,
            zIndex: 6
        })
        .add();

    var curPeriod = chart.renderer.label('', 53, 80, 'callout')
        .css({
            // color: '#FFFFFF'
            color: '#6f6f6f'
        })
        .attr({
            // fill: 'rgba(9, 1, 9, 0.50)',
            padding: 5,
            r: 10,
            zIndex: 6
        })
        .add();

    let conn = new ab.Session('ws://' + php_server_ip + ':8080',
        function () {
            conn.subscribe('candleData_' + localStorage.getItem("cur_symbol"), function (topic, data) {
                if (data.length && data.length > 0) {
                    for (var i = 0; i < data.length; i++) {
                        var mydata = data[i];
                        series.addPoint([mydata.data.time * 1000, 
                                parseFloat(parseFloat(mydata.data.open).toFixed(2)),
                                parseFloat(parseFloat(mydata.data.high).toFixed(2)),
                                parseFloat(parseFloat(mydata.data.low).toFixed(2)),
                                parseFloat(parseFloat(mydata.data.close).toFixed(2))],
                                false);
                    }
                } else {
                    series.addPoint([data.data.time * 1000,
                            parseFloat(parseFloat(data.data.open).toFixed(2)),
                            parseFloat(parseFloat(data.data.high).toFixed(2)),
                            parseFloat(parseFloat(data.data.low).toFixed(2)),
                            parseFloat(parseFloat(data.data.close).toFixed(2))],
                            false);
                }
                chart.redraw();
                curPeriod.attr({
                        text: '<span style="color: #6f6f6f; font-weight: bold;">'
                        + series.currentDataGrouping.count
                        + ' '
                        + series.currentDataGrouping.unitName
                        + (series.currentDataGrouping.count == 1 ? '' : 's')
                        });
            });
        },
        function () {
            console.warn('WebSocket connection closed');
        },
        { 'skipSubprotocolCheck': true }
    );

    // fix hichart over flow bug
    setTimeout(function () { chart.reflow(); }, 30);
});