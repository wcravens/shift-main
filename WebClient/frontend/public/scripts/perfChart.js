$(document).ready( function(){
  var EOD_INDEX = 6;
  var PROFITDETERMINATOR_INDEX = 4;
  var profitData = {};
  var contestStart = moment('2020-03-06 00:00:00', 'YYYY-MM-DD HH:mm:ss').valueOf();
  var allTimes = [];

  //leaderboardAllStats from userperf.php
  for(i = 0; i < leaderboardAllStats["data"].length; i++){
    var userTag = leaderboardAllStats["data"][i][0];
    if(!(userTag in profitData)){
      profitData[userTag] = []
    }
    
    var cTime = moment(leaderboardAllStats["data"][i][EOD_INDEX], 'YYYY-MM-DD HH:mm:ss');
    var normalized_cTime = moment(cTime.format('YYYY-MM-DD')+' 17:00:00');
    
    if(! allTimes.includes(normalized_cTime.valueOf())){

      allTimes.push(normalized_cTime.valueOf());

    }
    profitData[userTag].push([
                      parseFloat(leaderboardAllStats["data"][i][PROFITDETERMINATOR_INDEX])
                    ]);
  }

  var seriesData = [];
  var totalTeams = 0;
  var maxLen = 0;
  var buckets = []
  for(i = 1; i<allTimes.length+1; i++){
    buckets.push('Day ' + i)
  }
  console.log(buckets);

  for(var key in profitData){
    totalTeams++;
    allTimes.push()
    if(profitData[key].length > maxLen){
      maxLen = profitData[key].length;
    }

    seriesData.push({
      name: key,
      data: profitData[key],
    })
  }
  console.log(seriesData);
  //Calc average amt.
  
  profitData["avg"] = []
  for(i = 0; i < maxLen; i++){
    profitData["avg"].push(0.0)
  }

  for(var key in profitData){
    for(i = 0; i < profitData[key].length; i++){
      if(key == "avg"){
        continue;
      }
      profitData["avg"][i] += parseFloat(profitData[key][i]);

    }
  }



  console.log(maxLen+1);
  for(i = 0; i < maxLen; i++){
    console.log("SUM");
    console.log(profitData["avg"][i]);
    profitData["avg"][i] = profitData["avg"][i] / maxLen+1.0;
    console.log("AVG");
    console.log(profitData["avg"][i]);
  }

  //Separate the values into day categories
  seriesData.push({
      name: "avg",
      data: profitData["avg"],
  });


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
      labels: {
        formatter: function() {
          return buckets[this.value];
        }
      }
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
        line: {
          dataLabels: {
            enabled: true
          }
        },
        series: {
            marker: {
                enabled: true
            }
        },
        toolTip: {
          formatter: function (){
            return buckets[this.x] + ': ' + Highcharts.numberFormat(this.y,0);
          }
        }
    },
    series: seriesData
  });
});