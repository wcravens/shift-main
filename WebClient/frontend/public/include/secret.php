<?php
/*
* incredibly dumb php file for testing please god do not use this
*/
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$ghostValue = "boo~";

$traderList = "empty";
$traderList = ThriftClient::exec('\client\SHIFTServiceClient', 'getAllTraders');

?>

<div class="container">
<img src="http://itisamystery.com/iiam.gif" alt="">
<ul>
    <li>
        Mystery Values!
    </li>
    <li>
        <?php echo $traderList ?>
    </li>
</ul>
</div>
