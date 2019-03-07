<?php
/*
* navbar module
*/
require_once(getenv('SITE_ROOT').'/public/include/init.php');

$username = $user['username'];
if (isset($_GET['username'])) {
    $username = trim($_GET['username']);
}
?>

<nav class="navbar navbar-inverse navbar-fixed-top">
  <div class="container">
    <div class="navbar-header">
      <button type="button" class="navbar-toggle" data-toggle="collapse" data-target="#myNavbar">
        <span class="icon-bar"></span>
        <span class="icon-bar"></span>
        <span class="icon-bar"></span> 
      </button>
      <div class="navbar-brand notselectable white" data-toggle="popover" title="Shortcuts" data-placement="bottom" data-trigger="hover" data-html="true" 
        data-content='<table class="popovertable">
                      <tr>
                        <th style="padding:0 15px 0 15px;">Function</th>
                        <th style="padding:0 15px 0 15px;" align="center">Key</th>
                      </tr>
                      <tr height = 20px></tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Overview</td>
                        <td style="padding:0 15px 0 15px;" align="center">Q</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Order Book</td>
                        <td style="padding:0 15px 0 15px;" align="center">W</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">My Portfolio</td>
                        <td style="padding:0 15px 0 15px;" align="center">E</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Leaderboard</td>
                        <td style="padding:0 15px 0 15px;" align="center">R</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Toggle Dark Mode</td>
                        <td style="padding:0 15px 0 15px;" align="center">P</td>
                      </tr>
                      <tr height = 20px></tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Previous Stock</td>
                        <td style="padding:0 15px 0 15px;" align="center">←</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Next Stock</td>
                        <td style="padding:0 15px 0 15px;" align="center">→</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Price Input</td>
                        <td style="padding:0 15px 0 15px;" align="center">T</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Size Input</td>
                        <td style="padding:0 15px 0 15px;" align="center">G</td>
                      </tr>
                      <tr height = 20px></tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Market Buy</td>
                        <td style="padding:0 15px 0 15px;" align="center">A</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Market Sell</td>
                        <td style="padding:0 15px 0 15px;" align="center">S</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Limit Buy</td>
                        <td style="padding:0 15px 0 15px;" align="center">D</td>
                      </tr>
                      <tr>
                        <td style="padding:0 15px 0 15px;">Limit Sell</td>
                        <td style="padding:0 15px 0 15px;" align="center">F</td>
                      </tr>
                    </table>'>SHIFT Beta</div>
    </div>
    <div class="collapse navbar-collapse" id="myNavbar" >
      <ul class="nav navbar-nav">
        <li class="<?php echo $page=='overview'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='Best price summary for all tickers (Q)'><a href="/overview.php">Overview</a></li>
        <li class="<?php echo $page=='orderbook'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='Order book & candlestick chart (W)'><a href="/orderbook.php">Order Book</a></li>
        <li class="<?php echo $page=='myportfolio'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='Account summary & positions (E)'><a href="/myportfolio.php">My Portfolio</a></li>
        <li class="<?php echo $page=='leaderboard'?'active':''; ?> collapseitem" style="width: 110px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='Leaderboard (R)'><a href="/leaderboard.php">Leaderboard</a></li>
      </ul>
      <ul class="nav navbar-nav navbar-right">
        <!-- <li><a href="test.html"><i class="material-icons" style="font-size:22px;">lightbulb_outline</i></a></li> -->
        <!-- <li onclick="toggleFullScreen()"><a href="#"><i id="fullscreenbutton" class="material-icons" style="font-size:22px";>fullscreen</i></a></li> -->
        <p id="Summary" class="navbar-text notselectable notimp">BP: 0.00 | P&L: 0.00 (0.00%)</p> 
        <li class="dropdown collapseitem" style="width: 110px; text-align: center; margin-left: auto;">
          <a href="#" class="dropdown-toggle" data-toggle="dropdown" role="button" aria-haspopup="true" aria-expanded="false"><?php echo $username;?>&nbsp<span class="caret"></span></a>
          <ul class="dropdown-menu keep-open-on-click">
            <?php if ($userModel->is_admin()) { ?>
              <li><a href="/../admin">Admin Panel</a></li>
            <?php }?>
            <li>
              <h6 class="dropdown-header"><b>Quick Settings</b></h6>
            </li>
            <li id="lock-ticker">
              <a>
                <label class="switch">
                  Lock Ticker
                  <input id="lock-check" type="checkbox" checked onclick="togglelock()">
                  <span id="lock-slider" class="slider round"></span>
                </label>
              </a>
            </li>
            <li class="divider"></li>
            <li>
              <a href="../submittedorders.php">Submitted Orders</a>
            </li>
            <li>
              <a href="../myprofile.php">My Profile</a>
            </li>
            <li>
              <a href="../logout.php">Sign Out</a>
            </li>
          </ul>
        </li>
        <li class="collapseitem" onclick="toggleDarkTheme()" style="width: 55px; text-align: center; margin-left: auto;" data-toggle="popover" data-placement="bottom" data-trigger="hover" data-content='Toggle Dark Mode (P)'><a href="#"><i id="changethemebtn" class="material-icons" style="font-size:22px;">wb_sunny</i></a></li>
      </ul>
    </div>
  </div>
</nav>

<script type="text/javascript">
    var php_server_ip= "<?php echo $server_ip;?>";
    var php_username="<?php echo $username;?>";
    var php_shiftversion="<?php echo $SHIFT_version;?>";
</script>

<?php
echo '<script src="/scripts/nav.js?version='.$SHIFT_version.'"></script>';
?> 