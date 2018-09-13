<?php
// overview page
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$page = 'overview';
?>

<!DOCTYPE HTML>
<html lang="en">
    <head>
        <title>Overview - SHIFT Alpha</title>
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
            echo '<script src="/scripts/lastprice.js?version='.$SHIFT_version.'"></script>';
            echo '<script src="/scripts/overview.js?version='.$SHIFT_version.'"></script>';
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
                        <div class="stocks">
                            <table class="table notselectable bborder" id="stock_list">
                                <tr>
                                    <th width="5%">Symbol</th>
                                    <th class="text-right notwrap" width="19%">Last Price</th>
                                    <th class="text-right notwrap rborder" width="6%"></th>
                                    <th class="text-right notwrap notimpcol" width="13%">Bid Size</th>
                                    <th class="text-right notwrap" width="19%">Bid Price</th>
                                    <th class="text-right notwrap" width="19%">Ask Price</th>
                                    <th class="text-right notwrap notimpcol" width="19%">Ask Size</th>
                                </tr>
                                <?php  foreach ($stockList as $symbol) { ?>
                                <tr id="<?=$symbol ?>" class="clickable-row" >
                                    <td class="bold">
                                        <?=$symbol ?>
                                    </td>
                                    <td align="right"></td>
                                    <td align="right"></td>
                                    <td align="right"></td>
                                    <td align="right"></td>
                                    <td align="right"></td>
                                    <td align="right"></td>
                                </tr>
                                <?php } ?>
                            </table>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </body>
</html>