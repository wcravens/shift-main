<?php
// the leaderpage, which shows the current leaderboard. 
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$page = 'liveLeaderboard';
$redirect_url = '/liveLeaderboard.php';
include("./include/tablecheck.php");

?>

<!DOCTYPE HTML>
<html lang="en">
    <head>
        <title>Leaderboard - SHIFT Beta</title>
        <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
        <script src="/static/jquery-3.1.1.min.js"></script>  
        <script src="/static/autobahn.min.js"></script>
        <script src="/static/bootstrap/js/bootstrap.min.js"></script>
        <script type="text/javascript">
            var php_stockList_json = JSON.parse('<?php echo json_encode($stockList);?>');
            var php_server_ip= "<?php echo $server_ip;?>";
        </script>
        <?php
            echo '<script src="/scripts/verifications.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/keymapping.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/leaderboard.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/lastprice.js?version='.$SHIFT_version.'"></script>';
        ?> 
    </head>
    <body>
        <?php include("./include/nav.php");?>
        <div class="container">
            <div class="starter-template">
                <div class="row">
                    <div class="col-md-12">
                        <?php include_once('./include/lastprice.php');?>
                    </div>
                    <div class="col-md-12" style="padding-top: 10px;">
                        <h3 class="header3">Leaderboard</h3>
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
                        <div class="stocks">
                            <table class="table notselectable bborder nomargin" id="leaderboard">
                                <tr>
                                    <th class="text-right notwrap" width="5%">Rank</th>
                                    <th width="10%"></th>
                                    <th class="notwrap" width="10%">Username</th>
                                    <th class="text-right notwrap notimpcol" width="20%">Buying Power</th>
                                    <th class="text-right notwrap notimpcol" width="20%">Total Traded Shares</th>
                                    <th class="text-right notwrap notimpcol" width="20%">Total P&L</th>
                                    <th class="text-right notwrap" width="15%">Earnings</th>
                                </tr>
                            </table>
                        </div>
                        <div class="notselectable">
                            <font class="grey alignup" size="-2" >Rank is determined by the earnings of each trader. In case of a draw, the user who traded the less amount of shares is ranked first.</font>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </body>
</html>
