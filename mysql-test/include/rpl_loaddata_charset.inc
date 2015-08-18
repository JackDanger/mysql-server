connection primary;
--disable_warnings
DROP DATABASE IF EXISTS mysqltest;
--enable_warnings

CREATE DATABASE mysqltest CHARSET UTF8;
USE mysqltest;
CREATE TABLE t (cl varchar(100)) CHARSET UTF8;

if (!$LOAD_LOCAL)
{
  LOAD DATA INFILE '../../std_data/loaddata_utf8.dat' INTO TABLE t
  FIELDS TERMINATED BY ',' LINES TERMINATED BY '\n';
}
if ($LOAD_LOCAL)
{
  LOAD DATA LOCAL INFILE './std_data/loaddata_utf8.dat' INTO TABLE t
  FIELDS TERMINATED BY ',' LINES TERMINATED BY '\n';
}

save_primary_pos;
echo ----------content on primary----------;
SELECT hex(cl) FROM t;

connection replica;
sync_with_primary;
echo ----------content on replica----------;
USE mysqltest;
SELECT hex(cl) FROM t;

connection primary;
DROP DATABASE mysqltest;
save_primary_pos;
connection replica;
sync_with_primary;
