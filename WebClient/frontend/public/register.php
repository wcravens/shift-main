<?php
// basic register function
require_once(getenv('SITE_ROOT').'/public/include/init.php');
$user = new User();
$user->register($_POST['username'], $_POST['firstname'], $_POST['lastname'], $_POST['email'], $_POST['password'], $_POST['confirm_password']);
if (isset($_SESSION['err']) && !empty($_SESSION['err'])) {
    //should pass this err to next page
} else {
    $user->login_user($_POST['username'], $_POST['password']);
}
header("Location: /login.php?from=reg");