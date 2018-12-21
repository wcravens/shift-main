<?php
// submitted orders page
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$redirect_url = '/submittedorders.php';
?>

<!DOCTYPE HTML>
<html lang="en">
    <head>
        <title>Submitted Orders - SHIFT Beta</title>
        <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
        <script src="/static/jquery-3.1.1.min.js"></script>  
        <script src="/static/autobahn.min.js"></script>
        <script src="/static/bootstrap/js/bootstrap.min.js"></script>
        <script type="text/javascript">
            var php_stockList_json = JSON.parse('<?php echo json_encode($stockList);?>');
            var php_server_ip= "<?php echo $server_ip;?>";
            var php_username="<?php echo $username;?>"
        </script>
        <?php
            echo '<script src="/scripts/verifications.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/keymapping.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/lastprice.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/submittedorders.js?version='.$SHIFT_version.'"></script>';
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
                        <h3 class="header3">Submitted Orders</h3>
                        <table class="table notselectable bborder" id="stock_list">
                            <tr>
                                <th class="notwrap" width="5%">Symbol</th>
                                <th class="text-right notwrap" width="14%">Order Type</th>
                                <th class="text-right notwrap" width="9.5%">Price</th>
                                <th class="text-right notwrap" width="9.5%">Size</th>
                                <th width="5%"></th>
                                <th class="notwrap notimpcol2" width="35%">Order ID</th>
                                <th class="notwrap notimpcol2" width="35%">Timestamp</th>
                                <th class="text-right notwrap notimpcol" width="20%">Accepted</th>
                                <th class="text-right notwrap notimpcol" width="20%">Cancelled</th>
                            </tr>
                        </table>
                    </div>
                </div>
            </div>
        </div>
    </body>
</html>