<?php
// the class for accessing db, this is the wrong way of doing this. Frontend should not access db itself. All checking should be in the BC
class DB
{
    private static $dbh;
    public static function get_dbh()
    {
        if (!empty(self::$dbh)) {
            return self::$dbh;
        }

        $db_config_path = getenv('SITE_ROOT')."/public/data/loginCredential.php";
        while (file_exists($db_config_path) == false) {
            sleep(1);
        }
        require_once($db_config_path);
        unlink(realpath($db_config_path));

        self::$dbh = new PDO($db_config['dsn'], $db_config['user'], $db_config['pass'], array(
            PDO::ATTR_PERSISTENT => true
        ));
        return self::$dbh;
    }
}