<?php
use Ratchet\ConnectionInterface;
use Ratchet\Wamp\WampServerInterface;

//WebSocket Application Messaging Protocol (WAMP)
class pusher implements WampServerInterface {
    /**
     * A lookup of all the topics clients have subscribed to
     */
    protected $subscribedTopics = array();
    protected $storedData = array();
    protected $stockList = array();
    protected $leaderboard = array(); //[username =>{total_BP, total_shares, totalPL}, ...]

    protected $connections;
    protected $zmqSocket;

    public function __construct() {
        // used to manage all connections may use in the furure
        $this->connections = new \SplObjectStorage;
    }

    public function getZmqSocket(&$outerIns) {
        $this->zmqSocket = $outerIns;
    }

    //this $conn is a WampConnection
    public function onSubscribe(ConnectionInterface $conn, $topic) {
        $this->subscribedTopics[$topic->getId()] = $topic;
        if(isset($this->storedData[$topic->getId()])){
            $conn->event( $topic->getId(), $this->storedData[$topic->getId()]); 
        }
        echo "send zmq req_ ";
        echo $topic->getId()."\n";
        $this->zmqSocket->send($topic->getId());
    }

    /**
     * @param string JSON'ified string we'll receive from ZeroMQ
     */
    public function onData($entry) {
        // var_dump($entry);
        $entryData = json_decode($entry, true);

        // stupid code avoid showing web user info
        if(strpos($entryData['category'], '_web') !== false ){
            return;
        }

        if(strpos($entryData['category'], 'portfolio') !== false ){
            if(strpos($entryData['category'], 'portfolioSummary_') !== false ){//summary
                $data = $this->calLeaderboard($entryData);
                $data = array('category'=>'leaderboard', 'data'=>$data);
                if (array_key_exists('leaderboard', $this->subscribedTopics)) {
                    $leaerboard_topic = $this->subscribedTopics['leaderboard'];
                    $leaerboard_topic->broadcast($data);
                }
            }else{ //portfolio
                $arr = array();
                foreach($entryData['data'] as $key => $value){
                    if($value['symbol'] != "") {
                        array_push($arr, $value);
                    }
                }
                $entryData['data'] = $arr;
            }
        }

        if($entryData['category'] == "loginCredentials") {
            $file_path = '../../public/data/loginCredentials.php';
            if (file_exists($file_path))
                unlink($file_path);
            if (!file_exists('../../public/data/')) {
                mkdir('../../public/data/', 0777, true);
            }
            file_put_contents($file_path, $entryData['data']['data']);
        }

        if($entryData['category'] == 'stockList'){
            $this->stockList = $entryData['data'];
            // delete stockList file if exists
            if (file_exists('../../public/data/stockList.data'))
                unlink('../../public/data/stockList.data');
            if (!file_exists('../../public/data/')) {
                mkdir('../../public/data/', 0777, true);
            }
            file_put_contents('../../public/data/stockList.data', serialize($this->stockList));
        }

        if($entryData['category'] == 'companyNames'){
            foreach($entryData['data'] as $companyName){
                foreach ($companyName as $key => $value) {
                    $value = preg_replace('/\'/', "?", $value);
                    $value = preg_replace('/\"/', "=", $value);
                    $value = preg_replace('/ \(.*?\)/',"",$value);
                    $this->companyNames[$key] = $value;
                }
            }
            // delete symbol_companyName_map file if exists
            if (file_exists('../../public/data/symbol_companyName_map.data'))
                unlink('../../public/data/symbol_companyName_map.data');
            if (!file_exists('../../public/data/')) {
                mkdir('../../public/data/', 0777, true);
            }
            file_put_contents('../../public/data/symbol_companyName_map.data', serialize($this->companyNames));
        }

        if(strpos($entryData['category'], 'waitingList_') !== false){
            $this->storedData[$entryData['category']] =  $entryData;
        }

        if(strpos($entryData['category'], 'submittedOrders_') !== false){
            $this->storedData[$entryData['category']] =  $entryData;
        }

        if(strpos($entryData['category'], 'candleData_') !== false ){
            // push to end of that array
            $this->storedData[$entryData['category']][] =  $entryData;
        }

        if(strpos($entryData['category'], 'lastPriceView_') !== false ){
        }

        // If the lookup topic object isn't set there is no one to publish to
        if (!array_key_exists($entryData['category'], $this->subscribedTopics)) {
            return;
        }

        $topic = $this->subscribedTopics[$entryData['category']];

        // re-send the data to all the clients subscribed to that category
        $topic->broadcast($entryData); //todo remove comment later
    }

    public function calLeaderboard($data){
        $res = array();
        $username = str_replace("portfolioSummary_", '', $data['category']);
        if($this->isUserInClass($username)){
            $this->leaderboard[$username] = $data['data']; 
        }
        ksort($this->leaderboard);
        uasort($this->leaderboard, 
            function($a, $b){
                if($a['earnings'] > $b['earnings']) return -1;
                if($a['earnings'] < $b['earnings']) return 1;
                if($a['total_shares'] > $b['total_shares']) return 1;
                if($a['total_shares'] < $b['total_shares']) return -1;
                return 0;
        });
        foreach($this->leaderboard as $username => $value){
            $value['username'] = $username;
            array_push($res, $value);
        }
        return $res;
    }

    public function isUserInClass($username){
        return true;//todo delete this later runxi
        $users = array();
        for($i =1; $i<=50; $i++){
            array_push($users, "user$i");
        }
        return in_array($username, $users);
    }

    public function onUnSubscribe(ConnectionInterface $conn, $topic) {
    }

    public function onOpen(ConnectionInterface $conn) {
        $this->connections->attach($conn);
        echo "\n".sizeof($this->connections)."\n";
    }

    public function onClose(ConnectionInterface $conn) {
        $this->connections->detach($conn);
    }

    public function onCall(ConnectionInterface $conn, $id, $topic, array $params) {
        // In this application if clients send data it's because the user hacked around in console
        $conn->callError($id, $topic, 'You are not allowed to make calls')->close();
    }

    public function onPublish(ConnectionInterface $conn, $topic, $event, array $exclude, array $eligible) {
        // In this application if clients send data it's because the user hacked around in console
        $conn->close();
    }
    
    public function onError(ConnectionInterface $conn, \Exception $e) {
    }
}
