<?php
// the leaderpage, which shows the current leaderboard. 
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$page = 'leaderboard';
$redirect_url = '/leaderboard.php';
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
                        <?php include_once('./include/sendorderform.php');?>
                        <?php include_once('./include/lastprice.php');?>
                    </div>
                    <div class="col-md-12" style="padding-top: 10px;">
                        <h3 class="header3">Leaderboard</h3>
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