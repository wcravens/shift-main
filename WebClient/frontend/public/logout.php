<?php
// basic function of logout
require_once(getenv('SITE_ROOT').'/public/include/init.php');
$userModel = new User();
$userModel->logout();
header("location: /login.php");
