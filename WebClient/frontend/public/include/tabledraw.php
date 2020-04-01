<?php
// the leaderpage, which shows the current leaderboard. 
require_once(getenv('SITE_ROOT').'/public/include/init.php');

include("./include/tablecheck.php");

?>

<div class="collapse navbar-collapse" id="myNavbar" >
    <ul class="nav navbar-nav">
    <li class="<?php echo $page=='liveLeaderboard'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View graph'><a href="/liveLeaderboard.php">Live Leaderboard</a></li>
    <li class="<?php echo $lbViewed=='chart'?'statsBar':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View graph'><a href="/userperf.php">Daily Statistics</a></li>
    <li class="<?php echo $_GET["day"]=='1'?'statsBar':''; ?> collapseitem <?php echo $presentLeaderboardD1==true?'hide':''; ?>" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day One results'><a href="/leaderboard.php?day=1">Day </br> One</a></li>
    <li class="<?php echo $_GET["day"]=='2'?'statsBar':''; ?> collapseitem <?php echo $presentLeaderboardD2==true?'hide':''; ?>" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Two results'><a href="/leaderboard.php?day=2">Day </br> Two</a></li>
    <li class="<?php echo $_GET["day"]=='3'?'statsBar':''; ?> collapseitem <?php echo $presentLeaderboardD3==true?'hide':''; ?>" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Three results'><a href="/leaderboard.php?day=3">Day </br> Three</a></li>
    <li class="<?php echo $_GET["day"]=='4'?'statsBar':''; ?> collapseitem <?php echo $presentLeaderboardD4==true?'hide':''; ?>" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Four results'><a href="/leaderboard.php?day=4">Day </br> Four</a></li>
    <li class="<?php echo $_GET["day"]=='5'?'statsBar':''; ?> collapseitem <?php echo $presentLeaderboardD5==true?'hide':''; ?>" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Five results'><a href="/leaderboard.php?day=5">Day </br> Five</a></li>
    <li class="<?php echo $_GET["day"]=='6'?'statsBar':''; ?> collapseitem <?php echo $presentLeaderboardD6==true?'hide':''; ?>" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Six results'><a href="/leaderboard.php?day=6">Day </br> Six</a></li>
    <li class="<?php echo $_GET["day"]=='7'?'statsBar':''; ?> collapseitem <?php echo $presentLeaderboardD7==true?'hide':''; ?>" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Six results'><a href="/leaderboard.php?day=7">Day </br> Seven</a></li>
</ul>
</div>

<?php
require_once(getenv('SITE_ROOT').'/public/include/init.php');


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

foreach ($leaderBoardData["data"] as $idx => $row) {
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
