<?php
/*
 * cancel order form, used to cancel order
 */
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$userModel = new User();
$user = null;
if (!($user = $userModel->is_login())) {
    header("location: /login.php");
}

if (!isset($_POST['symbol']) || !isset($_POST['orderType']) || !isset($_POST['size']) || !isset($_POST['orderId']) || !isset($_POST['price'])) {
    exit('invalid submit');
}

$orderType = '';
$orderSymbol = $_POST['symbol'];
$orderSize = $_POST['size'] + 0;
$orderPrice = $_POST['price'] + 0.0;
$orderId = $_POST['orderId'];

//1:LimitBuy 2:LimitSell 3:MarketBuy 4:MarketSell 5:CancelBid 6:CancelAsk
if ($_POST['orderType'] == "5") {
    $orderType = '5';
}

if ($_POST['orderType'] == "6") {
    $orderType = '6';
}

ThriftClient::exec('\client\SHIFTServiceClient', 'submitOrder', array($user['username'], $orderType, $orderSymbol, $orderSize, $orderPrice, $orderId));
if (!empty($_POST['redirect_url'])) {
    header("Location: {$_POST['redirect_url']}");
} else {
    header("Location: /index.php");
}
