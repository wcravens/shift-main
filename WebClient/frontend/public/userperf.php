<?php
// the leaderpage, which shows the current leaderboard. 
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$page = 'leaderboard';
$lbViewed = 'chart';
$redirect_url = '/leaderboard.php';
$leaderBoardData = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboard', array('', '')));

?>

<!DOCTYPE HTML>
<html lang="en">
    <head>
        <title>Leaderboard - SHIFT Beta</title>
        <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
        <script src="/static/jquery-3.1.1.min.js"></script>  
        <script src="/static/autobahn.min.js"></script>
        <script src="/static/moment.js"></script>
        <script src="/static/bootstrap/js/bootstrap.min.js"></script>
        <script src="/static/highstock/js/highstock.js"></script>
        <script type="text/javascript">
            //Pass in the php values into javascript world (Kill me)
            var php_stockList_json = JSON.parse('<?php echo json_encode($stockList);?>');
            var php_server_ip= "<?php echo $server_ip;?>";
            var leaderboardAllStats = JSON.parse('<?php echo json_encode($leaderBoardData);?>');
        </script>
        <?php
            echo '<script src="/scripts/verifications.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/keymapping.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/perfChart.js"</script>';
            echo '<script src="/scripts/overview.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/lastprice.js?version='.$SHIFT_version.'"></script>';
        ?> 
    </head>
    <body>
        <?php include("./include/nav.php");?>
        <div class="container">
            <div class="starter-template">
                <div class="row">
                    <div class="col-md-12" style="padding-top: 10px;">
                        <h3 class="header3">Leaderboard</h3>
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
                        <div id="perfChart" style="width:100%; height:500px;"></div>
                        <div class="notselectable">
                            <font class="grey alignup" size="-2" >Rank is determined by the earnings of each trader. In case of a draw, the user who traded the less amount of shares is ranked first.</font>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </body>
</html>
<div id="perfChart" style="width:100%; height:400px;"></div>

