<?php
/*
 * sendorder form, used to send order
 */
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$userModel = new User();
$user = null;
if (!($user = $userModel->is_login())) {
    header("location: /login.php");
}

echo ($_POST['price']);

if (!isset($_POST['symbol']) || !isset($_POST['price']) || !isset($_POST['size']) || !isset($_POST['mysubmit'])) {
    exit('invalid submit');
}

$symbol = $_POST['symbol'];
$price = $_POST['price'] + 0.0;
$size = $_POST['size'] + 0;
$orderType = '';

//1:LimitBuy 2:LimitSell 3:MarketBuy 4:MarketSell 5:CancelBid 6:CancelAsk
if ($_POST['mysubmit'] == 'Limit Buy') {
    $orderType = '1';
}

if ($_POST['mysubmit'] == 'Limit Sell') {
    $orderType = '2';
}

if ($_POST['mysubmit'] == 'Market Buy') {
    $orderType = '3';
}

if ($_POST['mysubmit'] == 'Market Sell') {
    $orderType = '4';
}

ThriftClient::exec('\client\SHIFTServiceClient', 'submitOrder', array($symbol, "", $price, $size, $orderType, $user['username']));
if (!empty($_POST['redirect_url'])) {
    header("Location: {$_POST['redirect_url']}");
} else {
    header("Location: /index.php");
}
