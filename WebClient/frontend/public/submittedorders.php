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
                    <div class="col-md-12" style="padding-top: 10px;">
                        <h3 class="header3">Submitted Orders</h3>
                        <table class="table notselectable bborder" id="stock_list">
                            <tr>
                                <th class="notwrap" width="5%">Symbol</th>
                                <th class="text-right notwrap" width="10%">Order Type</th>
                                <th class="text-right notwrap" width="8%">Price</th>
                                <th class="text-right notwrap" width="8%">Size</th>
                                <th class="text-right notwrap" width="8%">Executed</th>
                                <th width="5%"></th>
                                <th class="notwrap notimpcol2" width="30%">Order ID</th>
                                <th class="notwrap notimpcol" width="10%">Status</th>
                                <th class="notwrap notimpcol2" width="20%">Timestamp</th>
                            </tr>
                        </table>
                    </div>
                </div>
            </div>
        </div>
    </body>
</html>
