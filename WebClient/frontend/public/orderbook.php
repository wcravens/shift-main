<?php
// orderbook page
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$page = 'orderbook';
$redirect_url = '/orderbook.php';
?>

<!DOCTYPE HTML>
<html lang="en">
    <head>
        <title>Order Book - SHIFT Beta</title>
        <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
        <script src="/static/jquery-3.1.1.min.js"></script>   
        <script src="/static/autobahn.min.js"></script>
        <script src="/static/highstock/js/highstock.js"></script>
        <script src="/static/bootstrap/js/bootstrap.min.js"></script>
        <script src="/static/jsTimezoneDetect/jstz.min.js"></script>
        <script type="text/javascript">
            var php_stockList_json = JSON.parse('<?php echo json_encode($stockList);?>');
            var php_symbol_companyName_map_json = JSON.parse('<?php echo json_encode($symbol_companyName_map); ?>');
            var php_server_ip= "<?php echo $server_ip;?>";
        </script>
        <?php
            echo '<script src="/scripts/verifications.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/keymapping.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/lastprice.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/orderbook.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/candledata.js?version='.$SHIFT_version.'"></script>';
        ?> 
    </head>
    <body>
        <?php include("./include/nav.php");?>
        <div class="container">
            <div class="starter-template">
                <div class="row">
                    <div class="col-md-12">
                        <div id="candle_data_container" style="position: relative"></div>
                        <div class="orderbook">
                            <div class="table-responsive col-xs-6 col-sm-6 col-md-6 col-lg-6">
                                <table class="table notselectable">
                                    <tr>
                                        <th colspan="6" class="text-center">Global</th>
                                    </tr>
                                    <tr>
                                        <th colspan="3" class="text-center">Bid</th>
                                        <th colspan="3" class="text-center">Ask</th>
                                    </tr>
                                    <tr>
                                        <td colspan="3">
                                            <table class="table bborder" id="global_bid_table">
                                                <tr>
                                                    <th class="text-right">Price</th>
                                                    <th class="text-right">Size</th>
                                                    <th width="22%"></th>
                                                    <!-- <th>Dest.</th> -->
                                                </tr>
                                            </table>
                                        </td>
                                        <td colspan="3">
                                            <table class="table bborder" id="global_ask_table">
                                                <tr>
                                                    <th class="text-right">Price</th>
                                                    <th class="text-right">Size</th>
                                                    <th width="22%"></th>
                                                    <!-- <th>Dest.</th> -->
                                                </tr>
                                            </table>
                                        </td>
                                    </tr>
                                </table>
                            </div>
                            <div class="table-responsive col-xs-6 col-sm-6 col-md-6 col-lg-6">
                                <table class="table notselectable">
                                    <tr>
                                        <th colspan="6" class="text-center">Local</th>
                                    </tr>
                                    <tr>
                                        <th colspan="3" class="text-center">Bid</th>
                                        <th colspan="3" class="text-center">Ask</th>
                                    </tr>
                                    <tr>
                                        <td colspan="3">
                                            <table class="table bborder" id="local_bid_table">
                                                <tr>
                                                    <th class="text-right">Price</th>
                                                    <th class="text-right">Size</th>
                                                    <th width="22%"></th>
                                                    <!-- <th>Dest.</th> -->
                                                </tr>
                                            </table>
                                        </td>
                                        <td colspan="3">
                                            <table class="table bborder" id="local_ask_table">
                                                <tr>
                                                    <th class="text-right">Price</th>
                                                    <th class="text-right">Size</th>
                                                    <th width="22%"></th>
                                                    <!-- <th>Dest.</th> -->
                                                </tr>
                                            </table>
                                        </td>
                                    </tr>
                                </table>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </body>
</html>
