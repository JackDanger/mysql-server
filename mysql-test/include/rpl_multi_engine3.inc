#############################################################
# Author: JBM
# Date: 2006-02-23
# Purpose: To reuse through out test and make maint easier
#############################################################
connection primary;
--echo "--- Insert into t1 --" as "";

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
select id,hex(b1),vc,bc,d,f,total,y,t from t1 order by id;

sync_replica_with_primary;
--echo --- Select from t1 on replica ---
select id,hex(b1),vc,bc,d,f,total,y,t from t1 order by id;

connection primary;

--echo --- Update t1 on primary --
UPDATE t1 SET b1 = 0, t="2006-02-22" WHERE id = 412;

--echo --- Check the update on primary --- 
SELECT id,hex(b1),vc,bc,d,f,total,y,t FROM t1 WHERE id = 412;

# Must give injector thread a little time to get update
# into the binlog other wise we will miss the update.

sync_replica_with_primary;
--echo --- Check Update on replica ---
SELECT id,hex(b1),vc,bc,d,f,total,y,t FROM t1 WHERE id = 412;

connection primary;
--echo --- Remove a record from t1 on primary ---
DELETE FROM t1 WHERE id = 42;

--echo --- Show current count on primary for t1 ---
SELECT COUNT(*) FROM t1;

sync_replica_with_primary;
--echo --- Show current count on replica for t1 --- 
SELECT COUNT(*) FROM t1;

connection primary;
DELETE FROM t1;
