<?php

$redirect_url = '/leaderboard.php';
$leaderBoardData = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array((int)$_GET["day"])), true);
$leaderBoardDataD1 = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array(1)), true);
$leaderBoardDataD2 = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array(2)), true);
$leaderBoardDataD3 = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array(3)), true);
$leaderBoardDataD4 = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array(4)), true);
$leaderBoardDataD5 = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array(5)), true);
$leaderBoardDataD6 = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array(6)), true);
$leaderBoardDataD7 = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'getThisLeaderboardByDay', array(7)), true);

$presentLeaderboardD1 = $leaderBoardDataD1["rowCount"] == 0;
$presentLeaderboardD2 = $leaderBoardDataD2["rowCount"] == 0;
$presentLeaderboardD3 = $leaderBoardDataD3["rowCount"] == 0;
$presentLeaderboardD4 = $leaderBoardDataD4["rowCount"] == 0;
$presentLeaderboardD5 = $leaderBoardDataD5["rowCount"] == 0;
$presentLeaderboardD6 = $leaderBoardDataD6["rowCount"] == 0;
$presentLeaderboardD7 = $leaderBoardDataD7["rowCount"] == 0;

?>