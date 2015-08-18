##################################################################
# Author: Giuseppe, Chuck Bell                                   #
# Date: 17-January-2007                                          #
# Purpose: To test that event effects are replicated             #
# in both row based and statement based format                   #
##################################################################

--disable_warnings
DROP EVENT IF EXISTS test.justonce;
drop table if exists t1,t2;
--enable_warnings

# first, we need a table to record something from an event

eval CREATE TABLE `t1` (
  `id` INT(10) UNSIGNED NOT NULL,
  `c` VARCHAR(50) NOT NULL,
  `ts` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=$engine_type DEFAULT CHARSET=utf8;

INSERT INTO t1 (id, c) VALUES (1, 'manually');

# We create the event so that it inserts exactly 1 row in the table
# A recuring event is used so that we can be sure the event will
# fire regardless of timing delays on the server. Otherwise, it is
# possible for the event to timeout before it has inserted a row.
--echo "Creating event test.justonce on the primary"
CREATE EVENT test.justonce ON SCHEDULE EVERY 2 SECOND DO 
  INSERT IGNORE INTO t1 (id, c) VALUES (2, 'from justonce');

# Show the event is alive and present on primary
--echo "Checking event is active on primary"
SELECT db, name, status, originator FROM mysql.event WHERE db = 'test' AND name = 'justonce';

# Wait until event has fired. We know this because t1 will contain 
# the row from the event.
let $wait_condition=
  SELECT COUNT(*) = 1 FROM t1 WHERE c = 'from justonce';
--source include/wait_condition.inc

# check that table t1 contains something
--echo "Checking event data on the primary"
let $events_done=`SELECT count(*)  FROM t1 id`;
--disable_query_log
eval SELECT $events_done > 0 as ONE;
--enable_query_log

sync_replica_with_primary;

--echo "Checking event data on the replica"
--disable_query_log
eval SELECT count(*) - $events_done as ZERO FROM t1 id;
--enable_query_log

--echo "Checking event is inactive on replica"
SELECT db, name, status, originator FROM mysql.event WHERE db = 'test' AND name = 'justonce';

# Create an event on the replica and check to see what the originator is.
--echo "Dropping event test.replica_once on the replica"
--disable_warnings
DROP EVENT IF EXISTS test.replica_once;
--enable_warnings

# Create an event on replica and check its state. An event shouldn't be executed 
# so set start time in 1 hour.
CREATE EVENT test.replica_once ON SCHEDULE EVERY 5 MINUTE STARTS CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO 
  INSERT IGNORE INTO t1(id, c) VALUES (3, 'from replica_once');

--echo "Checking event status on the replica for originator value = replica's server_id"
SELECT db, name, status, originator FROM mysql.event WHERE db = 'test' AND name = 'replica_once';

--echo "Dropping event test.replica_once on the replica"
--disable_warnings
DROP EVENT IF EXISTS test.replica_once;
--enable_warnings

connection primary;

# BUG#20384 - disable events on replica
--echo "Dropping event test.justonce on the primary"
--disable_warnings
DROP EVENT IF EXISTS test.justonce;
--enable_warnings

# Create an event on primary and check its state on replica. An event shouldn't be executed 
# so set start time in 1 hour. Check that changes of event statement replicated to replica

--echo "Creating event test.er on the primary"
CREATE EVENT test.er ON SCHEDULE EVERY 3 SECOND STARTS CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO 
  INSERT IGNORE INTO t1(id, c) VALUES (4, 'from er');

--echo "Checking event status on the primary"
SELECT db, name, status, originator, body FROM mysql.event WHERE db = 'test' AND name = 'er';

sync_replica_with_primary;

--echo "Checking event status on the replica"
SELECT db, name, status, originator, body FROM mysql.event WHERE db = 'test' AND name = 'er';

connection primary;
--echo "Altering event test.er on the primary"
ALTER EVENT test.er ON SCHEDULE EVERY 5 SECOND STARTS CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO 
  INSERT IGNORE INTO t1(id, c) VALUES (5, 'from alter er');

--echo "Checking event status on the primary"
SELECT db, name, status, originator, body FROM mysql.event WHERE db = 'test' AND name = 'er';

sync_replica_with_primary;

--echo "Checking event status on the replica"
SELECT db, name, status, originator, body FROM mysql.event WHERE db = 'test' AND name = 'er';

connection primary;
--echo "Dropping event test.er on the primary"
DROP EVENT test.er;

--echo "Checking event status on the primary"
SELECT db, name, status, originator FROM mysql.event WHERE db = 'test';

--disable_info

sync_replica_with_primary;

--echo "Checking event status on the replica"
SELECT db, name, status, originator FROM mysql.event WHERE db = 'test';

# test the DISABLE ON REPLICA for setting event REPLICASIDE_DISABLED as status
# on CREATE EVENT

# Create an event on replica and check its status. An event shouldn't be executed 
# so set start time in 1 hour.

--echo "Creating event test.replica_terminate on the replica"
CREATE EVENT test.replica_terminate ON SCHEDULE EVERY 3 SECOND STARTS CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO 
  INSERT IGNORE INTO t1(id, c) VALUES (6, 'from replica_terminate');

--echo "Checking event status on the replica"
SELECT db, name, status, originator FROM mysql.event WHERE db = 'test' AND name = 'replica_terminate';

--echo "Dropping event test.replica_terminate on the replica"
DROP EVENT test.replica_terminate;

--echo "Creating event test.replica_terminate with DISABLE ON REPLICA on the replica"
CREATE EVENT test.replica_terminate ON SCHEDULE EVERY 3 SECOND DISABLE ON REPLICA DO 
  INSERT IGNORE INTO t1(c) VALUES (7, 'from replica_terminate');

--echo "Checking event status on the replica"
SELECT db, name, status, originator FROM mysql.event WHERE db = 'test' AND name = 'replica_terminate';

--echo "Dropping event test.replica_terminate on the replica"
DROP EVENT test.replica_terminate;

--echo "Cleanup"
connection primary;
DROP TABLE t1;
sync_replica_with_primary;
connection primary;

