<?php
/*
 * sendorder form, used to send order
 */
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$userModel = new User();
$user = null;
if (!($user = $userModel->is_loginv2())) {
    header("location: /login.php");
}
$profile = $user[0];

echo($_POST['price']);

if (!isset($_POST['symbol']) || !isset($_POST['price']) || !isset($_POST['size']) || !isset($_POST['mysubmit'])) {
    exit('invalid submit');
}

$orderType = '';
$orderSymbol = $_POST['symbol'];
$orderSize = $_POST['size'] + 0;
$orderPrice = $_POST['price'] + 0.0;

// 1:LimitBuy 2:LimitSell 3:MarketBuy 4:MarketSell 5:CancelBid 6:CancelAsk
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

ThriftClient::exec('\client\SHIFTServiceClient', 'submitOrder', array($profile['username'], $orderType, $orderSymbol, $orderSize, $orderPrice, ''));
if (!empty($_POST['redirect_url'])) {
    header("Location: {$_POST['redirect_url']}");
} else {
    header("Location: /index.php");
}
