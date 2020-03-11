<div class="collapse navbar-collapse" id="myNavbar" >
    <ul class="nav navbar-nav">
        <li class="<?php echo $page=='Contest Performance'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View graph'><a href="/userperf.php">Daily Statistics</a></li>
        <li class="<?php echo $page=='Day One'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day One results'><a href="/leaderboard.php?start=2020-03-06&end=2020-03-07">Day One</a></li>
        <li class="<?php echo $page=='Day Two'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Two results'><a href="/leaderboard.php?start=2020-03-16&end=2020-03-17">Day Two</a></li>
        <li class="<?php echo $page=='Day Three'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Three results'><a href="/leaderboard.php?start=2020-03-26&end=2020-03-27">Day Three</a></li>
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
echo '<th class="notwrap" width="15%">Username</th>';
echo '<th class="text-right notwrap" width="5%">Rank</th>';
echo '<th class="text-right notwrap notimpcol" width="15%">EOD Buying Power</th>';
echo '<th class="text-right notwrap notimpcol" width="15%">EOD Total Traded Shares</th>';
echo '<th class="text-right notwrap notimpcol" width="15%">Total P&L</th>';
echo '<th class="text-right notwrap" width="15%">EOD Earnings</th>';
echo '<th class="text-right notwrap width="15%">EOD Time</th>';
echo '</tr>';

foreach ($leaderBoardData->data as $idx => $row) {
  echo '<tr>';

  foreach ($row as $value) {
    echo '<td class="text-right notimpcol">';
    echo json_encode($value);
    echo '</td>';  
  }
  echo '</tr>';
}
echo '</table>'
?>
