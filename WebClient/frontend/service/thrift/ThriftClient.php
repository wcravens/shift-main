<?php
error_reporting(E_ALL);

require_once getenv('SITE_ROOT').'/service/thrift/lib/Thrift/ClassLoader/ThriftClassLoader.php';

use Thrift\ClassLoader\ThriftClassLoader;

$GEN_DIR = getenv('SITE_ROOT').'/service/thrift/gen-php';

$loader = new ThriftClassLoader();
$loader->registerNamespace('Thrift', getenv('SITE_ROOT').'/service/thrift/lib');
$loader->registerNamespace('client', $GEN_DIR);
$loader->register();


use Thrift\Protocol\TBinaryProtocol;
use Thrift\Transport\TSocket;
use Thrift\Transport\THttpClient;
use Thrift\Transport\TBufferedTransport;
use Thrift\Exception\TException;

class ThriftClient
{
    public static function exec($clientName, $method, $params = array())
    {
        $ret = false;
        try {
            $socket = new TSocket('localhost', 9090);
            $transport = new TBufferedTransport($socket, 1024, 1024);
            $protocol = new TBinaryProtocol($transport);
            // $client = new \client\HelloWorldClient($protocol);
            $client = new $clientName($protocol);

            $transport->open();

            // $arr = $client->hello();
            // var_dump($arr);
            // $str = $client->getOrderBookJson('JPM');
            $ret = call_user_func_array(array($client, $method), $params);

            $transport->close();
        } catch (TException $tx) {
            // echo "<font color=\"blue\">blue</font>!";
            // print 'TException: '.$tx->getMessage()."\n";
            header("Location: error.php"); // redirect browser
            exit();
        }

        return $ret;
    }

    public static function connection_good($clientName)
    {
        try {
            $socket = new TSocket('localhost', 9090);
            $transport = new TBufferedTransport($socket, 1024, 1024);
            $protocol = new TBinaryProtocol($transport);
            // $client = new \client\HelloWorldClient($protocol);
            $client = new $clientName($protocol);
            $transport->open();
            $transport->close();
        } catch (TException $tx) {
            header("Location: error.php"); // redirect browser
            exit();
        }
    }
}
