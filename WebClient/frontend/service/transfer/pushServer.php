<?php
/*
 *  push-server works as ZMQ server, which receives requests from cpp ZMQ client and donot reply
 */
include_once dirname(__DIR__) . '/../vendor/autoload.php';
include_once dirname(__DIR__) . '/transfer/src/Pusher.class.php';

$pusher = new pusher();
$loop = React\EventLoop\Factory::create();
$context = new React\ZMQ\Context($loop);

echo "Connecting to hello world server…\n";
$requester = $context->getSocket(ZMQ::SOCKET_REQ);
$requester->bind("tcp://127.0.0.1:5550");
echo "Connected\n";
$requester->send("Hello");
$requester->on('message', function ($msg) use ($requester) {
    echo "receive from CC: ";
    echo $msg;
});
$pusher->getZmqSocket($requester);

$backend_overview_socket = $context->getSocket(ZMQ::SOCKET_PULL);
$backend_overview_socket->bind('tcp://127.0.0.1:5555');
$backend_overview_socket->on('error', function ($e) {
    var_dump($e->getMessage());
});
$backend_overview_socket->on('message', array($pusher, 'onData'));

// Set up our WebSocket server for clients wanting real-time updates
$web_socket = new React\Socket\Server($loop);
$web_socket->listen(8080, '0.0.0.0'); // Binding to 0.0.0.0 means remotes can connect
$web_server = new Ratchet\Server\IoServer(
    new Ratchet\Http\HttpServer(
        new Ratchet\WebSocket\WsServer(
            new Ratchet\Wamp\WampServer(
                $pusher
            )
        )
    ),
    $web_socket
);

$loop->run();
