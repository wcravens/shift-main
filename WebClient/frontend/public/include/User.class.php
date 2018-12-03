<?php
class User
{

    public function register($username, $firstname, $lastname, $email, $password, $confirm_password)
    {
        if (empty($username) || empty($firstname) || empty($lastname) || empty($email) || empty($password) ||empty($confirm_password)) {
            return $_SESSION['err'] = 'Please fill in all the fields below.';
        }
        $users = $this->get_user_by_username($username);
        if (count($users) != 0) {
            return $_SESSION['err'] = 'Please choose another username.';
        }
        if ($password != $confirm_password) {
            return $_SESSION['err'] = 'Provided passwords do not match.';
        }
        $dbh = DB::get_dbh();
        $sql = 'INSERT INTO new_traders (username, password, firstname, lastname, email) VALUES (?, ?, ?, ?, ?)';
        $sth = $dbh->prepare($sql);
        $is_succ = $sth->execute(array($username, sha1($password), $firstname, $lastname, $email));
        if ($is_succ == false) {
            return $_SESSION['err'] = 'Looks like we\'re having some server issues. Please try again later.';
        }
    }

    public function login_user($username, $password)
    {
        if ($this->is_login()) {
            return true;
        }

        if (empty($username) || empty($password)) {
            return $_SESSION['err'] = 'Please provide both username and password.';
        }
        $dbh = DB::get_dbh();
        $sql = 'SELECT * FROM new_traders WHERE username = ? AND password = ?';
        $sth = $dbh->prepare($sql);
        $sth->execute(array($username, sha1($password)));
        $rows = $sth->fetchAll();
        if (count($rows) != 1) {
            return $_SESSION['err'] = 'Your username or password is incorrect. Try again.';
        }
        $sessionid = $this->create_sessionid();

        //save sessionid into db
        $save_sessionid_sql = 'UPDATE new_traders SET sessionid = ? WHERE username = ?';
        $sth2 = $dbh->prepare($save_sessionid_sql);
        $is_succ = $sth2->execute(array($sessionid, $username));
        if ($is_succ == false) {
            return $_SESSION['err'] = 'Looks like we\'re having some server issues. Please try again later.';
        }

        //save sessionid to cookie
        setcookie("sessionid", $sessionid, time()+86400);
        return true;
    }

    public function is_login()
    {

        if (empty($_COOKIE['sessionid'])) {
            return false;
        }

        $sessionid = $_COOKIE['sessionid'];
        $dbh = DB::get_dbh();
        $sql = 'SELECT * FROM new_traders WHERE sessionid = ?';

        $sth = $dbh->prepare($sql);
        $sth->execute(array($sessionid));
        $rows = $sth->fetchAll();

        if (count($rows) != 1) {
            setcookie("sessionid", "", time() - 3600);
            return false;
        }

        return $rows[0];
    }

    
    public function create_sessionid()
    {
        return gen_uuid();
    }

    public function get_user_by_username($username)
    {
        $dbh = DB::get_dbh();
        $sql = 'SELECT * FROM new_traders WHERE username = ?';
        $sth = $dbh->prepare($sql);
        $sth->execute(array($username));
        return $sth->fetchAll();
    }

    public function is_admin()
    {
        $user = $this->is_login();
        if (empty($user)) {
            return false;
        }

        return $user['role'] == 'admin';
    }


    public function logout()
    {
        setcookie("sessionid", "", time() - 3600);
    }

}
