<?php
// login page
session_start();

require_once(getenv('SITE_ROOT').'/public/include/functions.php');
require_once(getenv('SITE_ROOT').'/public/include/Db.class.php');
require_once(getenv('SITE_ROOT').'/public/include/User.class.php');
require_once(getenv('SITE_ROOT').'/service/thrift/ThriftClient.php');

$user = new User();

if ($user->is_login()) {
    header("location: /index.php");
}

if (isset($_POST['login-submit'])) {
    $isSuccess = $user->login_user($_POST['username'], $_POST['password']);
    if ($isSuccess === true) {
        error_log(print_r($_POST['username'], true), 3, "/tmp/error.log");
        ThriftClient::exec('\client\SHIFTServiceClient', 'webUserLogin', array(trim($_POST['username'])));
        header("Location: /index.php");
    }
}

$fromReg = $_GET['from'] == 'reg';

if (isset($_SESSION['err']) && !empty($_SESSION['err'])) {
    $err = $_SESSION['err'];
    unset($_SESSION['err']);
}
?>

<!DOCTYPE html>
<html>
    <head>
        <title>SHIFT Beta</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <!-- <link rel="stylesheet" href="/static/bootstrap/css/bootstrap.min.css"> -->
        <link rel="stylesheet" href="/static/bootstrap/css/bootstrap.css">
        <link rel="stylesheet" href="/static/bootstrap/css/bootstrap-theme.min.css">
        <?php
            echo '<link rel="stylesheet" href="/style/login.css?version='.$SHIFT_version.'"></script>';
        ?> 
        <script src="/static/jquery-3.1.1.min.js"></script>  
        <script>
            $(function () {
                $('#login-form-link').click(function (e) {
                    $("#login-form").delay(100).fadeIn(100);
                    $("#register-form").fadeOut(100);
                    $('#register-form-link').removeClass('active');
                    $(this).addClass('active');
                    e.preventDefault();
                });
                $('#register-form-link').click(function (e) {
                    $("#register-form").delay(100).fadeIn(100);
                    $("#login-form").fadeOut(100);
                    $('#login-form-link').removeClass('active');
                    $(this).addClass('active');
                    e.preventDefault();
                });
                <?php
                if ($fromReg) {
                ?>
                    $("#register-form").delay(100).fadeIn(100);
                    $("#login-form").fadeOut(100);
                    $('#login-form-link').removeClass('active');
                    $('#register-form-link').addClass('active');
                <?php
                }
                ?>
            });
        </script>
    </head>
    <body>
        <div class="container">
            <div class="row">
                <?php if (isset($err)) {?>
                <div class="col-lg-6 col-lg-offset-3">
                    <div class="alert alert-danger alert-dismissible" role="alert"><button type="button" class="close" data-dismiss="alert"><span aria-hidden="true">×</span><span class="sr-only">Close</span></button>
                        <?php echo $err;?>
                    </div>
                </div>
                <?php }?>
                <div class="col-md-6 col-md-offset-3">
                    <div class="panel panel-login">
                        <div class="panel-heading">
                            <div class="row">
                                <div class="col-xs-6">
                                    <a href="#" class="active" id="login-form-link">Sign in</a>
                                </div>
                                <div class="col-xs-6">
                                    <a href="#" id="register-form-link">Sign up</a>
                                </div>
                            </div>
                            <hr>
                        </div>
                        <div class="panel-body">
                            <div class="row">
                                <div class="col-lg-12">
                                    <form id="login-form" action="/login.php" method="post" role="form" style="display: block;">
                                        <div class="form-group">
                                            <input type="text" name="username" id="username-login" tabindex="1" class="form-control" placeholder="Username" value="">
                                        </div>
                                        <div class="form-group">
                                            <input type="password" name="password" id="password-login" tabindex="2" class="form-control" placeholder="Password">
                                        </div>
                                        <div class="form-group text-center">
                                            <input type="checkbox" tabindex="3" class="" name="remember" id="remember">
                                            <label for="remember">Keep me signed in</label>
                                        </div>
                                        <div class="form-group">
                                            <div class="row">
                                                <div class="col-sm-6 col-sm-offset-3">
                                                    <input type="submit" name="login-submit" id="login-submit" tabindex="4" class="form-control btn btn-login" value="Sign in">
                                                </div>
                                            </div>
                                        </div>
                                        <div class="form-group">
                                            <div class="row">
                                                <div class="col-lg-12">
                                                    <div class="text-center">
                                                        <a tabindex="5" class="forgot-password">Forgot my password</a>
                                                    </div>
                                                </div>
                                            </div>
                                        </div>
                                    </form>
                                    <form id="register-form" action="/register.php" method="post" role="form" style="display: none;">
                                        <div class="form-group">
                                            <input type="text" name="username" id="username-reg" tabindex="1" class="form-control" placeholder="Username" value="">
                                        </div>
                                        <div class="form-group">
                                            <input type="text" name="firstname" id="firstname" tabindex="1" class="form-control" placeholder="First Name" value="">
                                        </div>
                                        <div class="form-group">
                                            <input type="text" name="lastname" id="lastname" tabindex="1" class="form-control" placeholder="Last Name" value="">
                                        </div>
                                        <div class="form-group">
                                            <input type="email" name="email" id="email" tabindex="1" class="form-control" placeholder="Email Address" value="">
                                        </div>
                                        <div class="form-group">
                                            <input type="password" name="password" id="password-reg" tabindex="2" class="form-control" placeholder="Password">
                                        </div>
                                        <div class="form-group">
                                            <input type="password" name="confirm_password" id="confirm-password-reg" tabindex="2" class="form-control" placeholder="Confirm Password">
                                        </div>
                                        <div class="form-group">
                                            <div class="row">
                                                <div class="col-sm-6 col-sm-offset-3">
                                                    <input type="submit" name="register-submit" id="register-submit" tabindex="4" class="form-control btn btn-register" value="Sign up">
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