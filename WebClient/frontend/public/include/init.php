<!-- 
initialization file
import common js css 

basic functions (gen rand version # for auto refresh)

php global var initialization
-->

<!-- this is a bad code, however simply solve flickering problem (find a better way of doing this latter if you can) -->
<script>
    // set default val for localstorage
    if (localStorage.getItem('dark-theme-enabled') === null) {
        localStorage.setItem('dark-theme-enabled', false);
    }
    if (localStorage.getItem('tickerlocked') === null){
        localStorage.setItem('tickerlocked', true);
    }
    if (JSON.parse(localStorage.getItem('dark-theme-enabled')))
        document.write('<style type="text/css">body{display:none; background-color: black;}</style>');
    else
        document.write('<style type="text/css">body{display:none}</style>');

    var elm=document.getElementsByTagName("html")[0];
    elm.style.display="none";
    document.addEventListener("DOMContentLoaded", function(event) { elm.style.display="block"; });
</script>

<?php
/*
* Common init code
*/
session_start();

require_once(getenv('SITE_ROOT').'/public/include/functions.php');
require_once(getenv('SITE_ROOT').'/public/include/User.class.php');
require_once(getenv('SITE_ROOT').'/service/thrift/ThriftClient.php');

// currently using random string as version num (adding version to js & css files to force refresh)
$SHIFT_version = substr(md5(mt_rand()), 0, 7);

// check if user already login, if not redirect to login.php
$userModel = new User();
$user = null;
if (!($user = $userModel->is_loginv2())) {
    header("location: /login.php");
}
$profile = $user[0];

// register to BROKERAGECENTER, if WC not running, redirect to error.php
ThriftClient::exec('\client\SHIFTServiceClient', 'webUserLogin', array(trim($profile['username'])));

// the stocklist
$stockList = array_values(unserialize(file_get_contents(getenv('SITE_ROOT').'/public/data/stockList.data')));

// map from AAPL to Apple Inc.
$symbol_companyName_map = unserialize(file_get_contents(getenv('SITE_ROOT').'/public/data/symbol_companyName_map.data'));

// local ip
$server_ip = getLocalIp();

// pass symbol value from php to javascript
if (isset($_GET['symbol']) && $_GET['symbol']!='' && in_array($_GET['symbol'], $stockList)) {
    ?>
    <script>
        localStorage.setItem("cur_symbol", "<?php echo $_GET['symbol']; ?>");
    </script>
<?php
}

// overall css
echo '<link rel="stylesheet" href="/style/overall.css?version='.$SHIFT_version.'">';
// js functions
echo '<script src="/scripts/functions.js?version='.$SHIFT_version.'"></script>';
// google icons
echo '<link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">';
// jquery css
echo '<script src="/static/jquery-3.1.1.min.js"></script>';
