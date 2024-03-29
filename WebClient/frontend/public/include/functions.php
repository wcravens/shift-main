<?php
/*
* general php functions
*/

// return the local ip of nginx server
define('__VENDOR_ROOT__', getenv('SITE_ROOT').'/vendor/');
require_once(__VENDOR_ROOT__.'autoload.php');

$dotEnv = Dotenv\Dotenv::createImmutable('/usr/local/share/shift/WebClient/', 'php.env');
$dotEnv->load();

function getLocalIp()
{
    $envIP = $_ENV['WSADDR'];
    if ($envIP == "") {
        $envIP = $_SERVER['SERVER_ADDR'];
    }
    return $envIP;
    //return $_SERVER['SERVER_ADDR'];
}

// generate a uuid (currently used for identify order history, waiting list)
// dismissed
function gen_uuid()
{
    return sprintf(
        '%04x%04x-%04x-%04x-%04x-%04x%04x%04x',
        // 32 bits for "time_low"
        mt_rand(0, 0xffff),
        mt_rand(0, 0xffff),

        // 16 bits for "time_mid"
        mt_rand(0, 0xffff),

        // 16 bits for "time_hi_and_version",
        // four most significant bits holds version number 4
        mt_rand(0, 0x0fff) | 0x4000,

        // 16 bits, 8 bits for "clk_seq_hi_res",
        // 8 bits for "clk_seq_low",
        // two most significant bits holds zero and one for variant DCE1.1
        mt_rand(0, 0x3fff) | 0x8000,

        // 48 bits for "node"
        mt_rand(0, 0xffff),
        mt_rand(0, 0xffff),
        mt_rand(0, 0xffff)
    );
}
