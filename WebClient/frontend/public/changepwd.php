<?php
// my profile page
require_once(getenv('SITE_ROOT').'/public/include/init.php');

if (isset($_POST['change-password-submit'])) {
    $isSucc = $userModel->change_passwordv2($_POST['cur_password'], $_POST['new_password'], $_POST['confirm_new_password']);
    if ($isSucc === true) {
        echo '<script>console.log("isSucc")</script>';
        header("location: /myprofile.php");
    }
}

if (isset($_SESSION['err']) && !empty($_SESSION['err'])) {
    $err = $_SESSION['err'];
    unset($_SESSION['err']);
    $retrying = true;
}
?>

<!DOCTYPE HTML>
<html lang="en">
    <head>
        <title>Change Password - SHIFT Beta</title>
        <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
        <script src="/static/jquery-3.1.1.min.js"></script>
        <script src="/static/autobahn.min.js"></script>
        <script src="/static/bootstrap/js/bootstrap.min.js"></script>
        <script type="text/javascript">
            var php_stockList_json = JSON.parse('<?php echo json_encode($stockList);?>');
            var php_server_ip= "<?php echo $server_ip;?>";
        </script>
        <?php
            echo '<script src="/scripts/verifications.js?version='.$SHIFT_version.'"></script>';
        ?> 
        <?php
            echo '<link rel="stylesheet" href="/style/login.css?version='.$SHIFT_version.'"></script>';
        ?>

        <script>
            $(function () {
                $("#change-password-form").fadeIn(100);
            });
        </script>
    </head>
    <body>
        <?php include("./include/nav.php")?>
        <div class="container">
            <div class="row">
                <?php if (isset($err))
                {
                ?>
                    <div class="col-lg-6 col-lg-offset-3">
                        <div class="alert alert-danger alert-dismissible" role="alert"><button type="button" class="close" data-dismiss="alert"><span aria-hidden="true">×</span><span class="sr-only">Close</span></button>
                            <?php echo $err;?>
                        </div>
                    </div>
                <?php
                }
                ?>
                <div class="col-md-6 col-md-offset-3">
                    <div class="panel panel-login">
                        <div class="panel-body">
                            <div class="row">
                                <div class="col-lg-12">
                                    <form id="change-password-form" action="./changepwd.php" method="post" role="form" style="display: none;">
                                        <div class="form-group">
                                            <input type="password" name="cur_password" id="cur-password" tabindex="2" class="form-control" placeholder="Current Password" required>
                                        </div>
                                        <div class="form-group">
                                            <input type="password" name="new_password" id="new-password" tabindex="2" class="form-control" placeholder="New Password" required>
                                        </div>
                                        <div class="form-group">
                                            <input type="password" name="confirm_new_password" id="confirm-new-password" tabindex="2" class="form-control" placeholder="Confirm New Password" required>
                                        </div>
                                        <div class="form-group">
                                            <div class="row">
                                                <div class="col-sm-6 col-sm-offset-3">
                                                    <input type="submit" name="change-password-submit" id="change-password-submit" tabindex="4" class="form-control btn btn-login" value="Change Password">
                                                </div>
                                            </div>
                                        </div>
                                    </form>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </body>
</html>