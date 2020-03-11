$(document).ready( function(){
  var profitData = {};
  var contestStart = moment('2020-03-06 00:00:00', 'YYYY-MM-DD HH:mm:ss').valueOf();
  //leaderboardAllStats from userperf.php
  for(i = 0; i < leaderboardAllStats["data"].length; i++){
    var userTag = leaderboardAllStats["data"][i][0];
    if(!(userTag in profitData)){
      profitData[userTag] = []
    }
    
    profitData[userTag].push([
                      moment(leaderboardAllStats["data"][i][6], 'YYYY-MM-DD HH:mm:ss').valueOf(),
                      parseFloat(leaderboardAllStats["data"][i][2])
                    ]);
    console.log(profitData[userTag]);
  }

  var seriesData = [];
  for(var key in profitData){
    seriesData.push({
      name: key,
      data: profitData[key],
      pointStart: contestStart,
      pointInterval: 24 * 3600 * 1000
    })
  }
  var myChart = Highcharts.chart('perfChart', {
    chart: {
      type: 'line'
    },
    title: {
      text: 'Profit Evaluation'
    },
    yAxis: {
        title: {
          text: "Profit"
        }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: {
        day: '%e of %b'
      },
      title: {
        text: "Day"
      },
      tickInterval: 24 * 3600 * 1000 // interval of 1 day (in your case = 60)
    },
    legend: {
      title: {
        text: 'Teams<br/><span style="font-size: 9px; color: #666; font-weight: normal">(Click to hide)</span>',
        style: {
            fontStyle: 'italic'
        }
      },
      backgroundColor: '#FCFFC5',
      borderColor: '#C98657',
      borderWidth: 1
    },
    plotOptions: {
        series: {
            marker: {
                enabled: true
            }
        },
        column: {
          pointStart: contestStart
        }
    },
    series: seriesData
  });
});