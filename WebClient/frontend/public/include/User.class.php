<?php
require_once(getenv('SITE_ROOT').'/service/thrift/ThriftClient.php');
define('__VENDOR_ROOT__', getenv('SITE_ROOT').'/vendor/');
require_once(__VENDOR_ROOT__.'autoload.php');

$dotEnv = Dotenv\Dotenv::createImmutable('/usr/local/share/shift/WebClient/', 'php.env');
$dotEnv->load();

class User
{
    public function check_permit_all()
    {
        $permitFlag = $_ENV['PERMITALL'];

        return $permitFlag === "ALLOW";
    }

    public function registerv2($username, $firstname, $lastname, $email, $password, $confirm_password)
    {
        if (empty($username) || empty($firstname) || empty($lastname) || empty($email) || empty($password) ||empty($confirm_password)) {
            return $_SESSION['err'] = 'Please fill in all the fields below.';
        }

        $users = $this->get_user_by_usernamev2($username);
        if (count($users) != 0) {
            return $_SESSION['err'] = 'Please choose another username.';
        }
        if ($password != $confirm_password) {
            return $_SESSION['err'] = 'Provided passwords do not match.';
        }

        $profile = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'registerUser', array($username, $firstname, $lastname, $email, $password)), true);

        if ($profile["success"] == false) {
            return $_SESSION['err'] = 'Looks like we\'re having some server issues. Please try again later.';
        }
    }

    public function login_userv2($username, $password)
    {
        if ($this->is_loginv2()) {
            return true;
        }

        if (empty($username) || empty($password)) {
            return $_SESSION['err'] = 'Please provide both username and password.';
        }

        $profile = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'webUserLoginV2', array($username, $password)), true);

        setcookie("sessionid", $profile['sessionid'], time()+86400);
        return $profile['success'];
    }

    public function is_loginv2()
    {
        if (empty($_COOKIE['sessionid'])) {
            return false;
        }
        $sessionid = $_COOKIE['sessionid'];

        $profile = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'is_login', array($sessionid)), true);
        
        $result = [];
        if (!$profile['success']) {
            setcookie("sessionid", "", time() - 3600);
        } else {
            array_push($result, $profile);
        }

        return $result;
    }

    public function change_passwordv2($cur_password, $new_password, $confirm_new_password)
    {
        $user = $this->is_loginv2();
        if (count($user) == 0) {
            return $_SESSION['err'] = 'Could not reset PW.';
        }

        $profile = $user[0];

        $username = $profile['username'];

        // validate confirm_new_password and new_password
        if ($confirm_new_password != $new_password) {
            return $_SESSION['err'] = 'Provided passwords do not match.';
        }

        $changepw_result = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'change_password', array($cur_password, $new_password, $username)), true);
        
        if ($changepw_result["success"] == false) {
            return $_SESSION['err'] = 'Authorization Failed';
        }

        return true;
    }

    public function create_sessionid()
    {
        return gen_uuid();
    }


    public function get_user_by_usernamev2($username)
    {
        $profile = json_decode(ThriftClient::exec('\client\SHIFTServiceClient', 'get_user_by_username', array($username)), true);

        $results = [];

        if ($profile['success']) {
            array_push($results, $profile);
        }

        return $results;
    }

    public function is_adminv2()
    {
        $user = $this->is_loginv2();
        if (empty($user)) {
            return false;
        }

        $profile = $user[0];
        return $profile['role'] == 'admin' && ! $this->check_permit_all();
    }

    public function is_studentv2()
    {
        $user = $this->is_loginv2();
        if (empty($user)) {
            return false;
        }

        $profile = $user[0];
        return $profile['role'] == 'student' && !$this->check_permit_all();
    }

    public function logout()
    {
        setcookie("sessionid", "", time() - 3600);
    }
}
