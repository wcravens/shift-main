<?php
// the leaderpage, which shows the current leaderboard. 
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$page = 'leaderboard';
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
                    <div class="col-md-12">
                        <?php include_once('./include/sendorderform.php');?>
                        <?php include_once('./include/lastprice.php');?>
                    </div>
                    <div class="col-md-12" style="padding-top: 10px;">
                        <h3 class="header3">Leaderboard</h3>
                        <div class="collapse navbar-collapse" id="myNavbar" >
                            <ul class="nav navbar-nav">
                                <li class="<?php echo $page=='Contest Performance'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View graph'><a href="/userperf.php">Daily Statistics</a></li>
                                <li class="<?php echo $page=='Day One'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day One results'><a href="/leaderboard.php?start=2020-03-06&end=2020-03-07">Day One</a></li>
                                <li class="<?php echo $page=='Day Two'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Two results'><a href="/leaderboard.php?start=2020-03-16&end=2020-03-17">Day Two</a></li>
                                <li class="<?php echo $page=='Day Three'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='View Day Three results'><a href="/leaderboard.php?start=2020-03-26&end=2020-03-27">Day Three</a></li>
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

