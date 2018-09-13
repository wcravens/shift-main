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
        require_once(getenv('SITE_ROOT')."/../config/db_config.php");
        self::$dbh = new PDO($db_config['dsn'], $db_config['user'], $db_config['pass'], array(
            PDO::ATTR_PERSISTENT => true
        ));
        return self::$dbh;
    }
}