#############################################################
# Author: Rafal
# Date: 2007-08-20
# based on rpl_multi_engine3.inc
#############################################################

connection replica;
STOP REPLICA;
RESET REPLICA;

connection primary;
RESET PRIMARY;

connection replica;
START REPLICA;

--echo --- Populate t1 with data ---
connection primary;
--disable_query_log
INSERT INTO t1 VALUES(42,1,'Testing MySQL databases is a cool ',
                      'Must make it bug free for the customer',
                       654321.4321,15.21,0,1965,"1905-11-14");
INSERT INTO t1 VALUES(2,1,'Testing MySQL databases is a cool ',
                      'Must make it bug free for the customer',
                       654321.4321,15.21,0,1965,"1965-11-14");
INSERT INTO t1 VALUES(4,1,'Testing MySQL databases is a cool ',
                      'Must make it bug free for the customer',
                       654321.4321,15.21,0,1965,"1985-11-14");
INSERT INTO t1 VALUES(142,1,'Testing MySQL databases is a cool ',
                      'Must make it bug free for the customer',
                       654321.4321,15.21,0,1965,"1995-11-14");
INSERT INTO t1 VALUES(412,1,'Testing MySQL databases is a cool ',
                      'Must make it bug free for the customer',
                       654321.4321,15.21,0,1965,"2005-11-14");
--enable_query_log

--echo --- Select from t1 on primary --- 
select *
from t1 
order by id;

sync_replica_with_primary;
--echo --- Select from t1 on replica ---
select *
from t1 
order by id;

--echo --- Perform basic operation on primary ---
--echo --- and ensure replicated correctly ---
connection primary;

--echo --- Update t1 on primary --
UPDATE t1 SET b1 = 0, bc='updated', t="2006-02-22" 
WHERE id < 100 
ORDER BY id;

--echo --- Check the update on primary --- 
SELECT *
FROM t1 
WHERE id < 100
ORDER BY id;

# Must give injector thread a little time to get update
# into the binlog other wise we will miss the update.

sync_replica_with_primary;
--echo --- Check Update on replica ---
SELECT *
FROM t1 
WHERE id < 100
ORDER BY id;

connection primary;
--echo --- Remove a record from t1 on primary ---
# Note: there is an error in replication of Delete_row
# from NDB to MyISAM (BUG#28538). However, if there is
# only one row in Delete_row event then it works fine,
# as this test demonstrates.
DELETE FROM t1 WHERE id = 412;

--echo --- Show current count on primary for t1 ---
SELECT COUNT(*) FROM t1;

sync_replica_with_primary;
--echo --- Show current count on replica for t1 --- 
SELECT COUNT(*) FROM t1;

connection primary;
TRUNCATE TABLE t1;
sync_replica_with_primary;
connection primary;
