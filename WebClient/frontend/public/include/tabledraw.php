<div class="collapse navbar-collapse" id="myNavbar" >
    <ul class="nav navbar-nav">
    <li class="<?php echo $lbViewed=='chart'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View graph'><a href="/userperf.php">Daily Statistics</a></li>
    <li class="<?php echo $page=='liveLeaderboard'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View graph'><a href="/liveLeaderboard.php">Live Leaderboard</a></li>    
    <li class="<?php echo $_GET["start"]=='2020-03-06'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day One results'><a href="/leaderboard.php?start=2020-03-06&end=2020-03-07">Day </br> One</a></li>
    <li class="<?php echo $_GET["start"]=='2020-03-14'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Two results'><a href="/leaderboard.php?start=2020-03-14&end=2020-03-15">Day </br> Two</a></li>
    <li class="<?php echo $_GET["start"]=='2020-03-21'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Three results'><a href="/leaderboard.php?start=2020-03-21&end=2020-03-22">Day </br> Three</a></li>
    <li class="<?php echo $_GET["start"]=='2020-03-28'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Four results'><a href="/leaderboard.php?start=2020-03-28&end=2020-03-29">Day </br> Four</a></li>
    <li class="<?php echo $_GET["start"]=='2020-04-04'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Five results'><a href="/leaderboard.php?start=2020-04-04&end=2020-04-05">Day </br> Five</a></li>
    <li class="<?php echo $_GET["start"]=='2020-04-10'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Six results'><a href="/leaderboard.php?start=2020-04-10&end=2020-04-11">Day </br> Six</a></li>
    </ul>
</div>

<?php
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$leaderBoardData = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboard', array($_GET["start"], $_GET["end"])));

//Personally, this feels ~slightly~ more manageable than using jquery - J.U.
//especially since the tables aren't live anymore.
//Though I do understand why it was done w/ jquery..
echo '<table class="table notselectable bborder nomargin" id="leaderboard">';
echo '<tr>';
echo '<th class="text-right notwrap" width="5%">Rank</th>';
echo '<th class="text-right" width="15%">Username</th>';
echo '<th class="text-right notwrap notimpcol" width="15%">EOD Buying Power</th>';
echo '<th class="text-right notwrap notimpcol" width="15%">EOD Total Traded Shares</th>';
echo '<th class="text-right notwrap notimpcol" width="15%">Total P&L</th>';
echo '<th class="text-right notwrap" width="15%">EOD Earnings</th>';
echo '<th class="text-right notwrap width="15%">EOD Time</th>';
echo '</tr>';

foreach ($leaderBoardData->data as $idx => $row) {
  echo '<tr>';

  $idx = 0;
  foreach ($row as $value) {
    echo '<td class="text-right notimpcol">';
    if($idx == 2 || $idx == 4 || $idx == 5){
      echo number_format($value, 2);
    }
    else{
      echo $value;
    }
    echo '</td>';  
    $idx++;
  }
  echo '</tr>';
}
echo '</table>'
?>
