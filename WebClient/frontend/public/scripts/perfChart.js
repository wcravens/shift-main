$(document).ready( function(){
  var AVERAGE_LABEL = "Average";
  var EOD_INDEX = 6;
  var USERNAME_INDEX = 1
  var PROFITDETERMINATOR_INDEX = 4;
  var profitData = {};
  var contestStart = moment('2020-03-06 00:00:00', 'YYYY-MM-DD HH:mm:ss').valueOf();
  var allTimes = [];

  //leaderboardAllStats from userperf.php
  for(i = 0; i < leaderboardAllStats["data"].length; i++){
    var userTag = leaderboardAllStats["data"][i][USERNAME_INDEX];
    if(!(userTag in profitData)){
      profitData[userTag] = []
    }
    
    var cTime = moment(leaderboardAllStats["data"][i][EOD_INDEX], 'YYYY-MM-DD HH:mm:ss');
    var normalized_cTime = moment(cTime.format('YYYY-MM-DD')+' 17:00:00');
    
    if(! allTimes.includes(normalized_cTime.format('MM-DD'))){

      allTimes.push(normalized_cTime.format('MM-DD'));
    }
    var cVal = parseFloat(leaderboardAllStats["data"][i][PROFITDETERMINATOR_INDEX]);
    cVal = Math.round(cVal * 1e2) / 1e2;
    profitData[userTag].push([
                      cVal
                    ]);
  }

  var seriesData = [];
  var totalTeams = 0;
  var maxLen = 0;
  var buckets = []
  console.log("ALLTIMES" + allTimes);
  for(i = 0; i<allTimes.length; i++){
    buckets.push('Day ' + (i+1) + '<br></br>(' + allTimes[i] + ')')
    console.log('BUCKETS ' + buckets[i]);

  }
  console.log(buckets);

  for(var key in profitData){
    totalTeams++;
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
  
  profitData[AVERAGE_LABEL] = []
  for(i = 0; i < maxLen; i++){
    profitData[AVERAGE_LABEL].push(0.0)
  }

  for(var key in profitData){
    for(i = 0; i < profitData[key].length; i++){
      if(key == AVERAGE_LABEL){
        continue;
      }
      profitData[AVERAGE_LABEL][i] += parseFloat(profitData[key][i]);

    }
  }



  console.log(maxLen+1);
  for(i = 0; i < maxLen; i++){
    console.log("SUM");
    console.log(profitData[AVERAGE_LABEL][i]);
    profitData[AVERAGE_LABEL][i] = profitData[AVERAGE_LABEL][i] / maxLen+1.0;
    console.log("AVG");
    console.log(profitData[AVERAGE_LABEL][i]);
  }

  //Separate the values into day categories
  seriesData.push({
      name: AVERAGE_LABEL,
      data: profitData[AVERAGE_LABEL],
      color: '#FF0000'
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
          text: "P&L (USD)"
        }
    },
    xAxis: {
      labels: {
        formatter: function() {
          console.log(buckets[this.value]);
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
    tooltip: {
      formatter: function () {
        return buckets[this.x] + ' results: ' + Highcharts.numberFormat(this.y);
      }
    },
    plotOptions: {
      line: {
        dataLabels: {
          enabled: true,
          formatter: function() {
            if (this.isLast) {
              return this.value
            }
          }  
        },
      }
    },
    series: seriesData
  });
});