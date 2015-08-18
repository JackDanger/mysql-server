# WL#7796 - WAIT_FOR_EXECUTED_GTID_SET
#
# Bug#19179920  WAIT_FOR_EXECUTED_GTID_SET SOMETIMES DOES NOT WAIT
#
# This patch fixes the problem of the WAIT_FOR_EXECUTED_GTID_SET which was
# not waiting incase the timeout is not specified by the user.
#
# To run this test you need to define:
#  $gtid_wait_set              The set used to check the server is waiting

if ($gtid_wait_set == '')
{
  --die ERROR IN TEST: the "gtid_wait_set" variable must be set
}

--let $rpl_gtid_utils= 1
--source include/have_log_bin.inc
--source include/have_gtid.inc
--source include/primary-replica.inc

--connection replica
--let $primary_uuid= query_get_value(select SOURCE_UUID from performance_schema.replication_connection_status WHERE service_state = 'ON', SOURCE_UUID, 1)
--connection primary

--let $gtid_wait_set= $primary_uuid:$gtid_wait_set

--connection primary
CREATE TABLE t1 (c1 INT NOT NULL PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO t1 VALUES (1);

--connection primary1

CREATE USER `ABC`;
--let $connection_id= `SELECT CONNECTION_ID()`
--send
--replace_result $primary_uuid PRIMARY_UUID
--eval SELECT WAIT_FOR_EXECUTED_GTID_SET('$gtid_wait_set')

--connection default
# Checking the waiting thread condition.
--echo The connection primary1 should be waiting for the WAIT_FOR_EXECUTED_GTID_SET here
--let $wait_condition=SELECT COUNT(*)=1 FROM INFORMATION_SCHEMA.PROCESSLIST WHERE Id='$connection_id' AND Info Like 'SELECT WAIT_FOR_EXECUTED_GTID_SET%'
--source include/wait_condition.inc
# Now executing the next transaction.
INSERT INTO t1 values(2);

--connection primary1
--replace_result $primary_uuid PRIMARY_UUID
--reap

--let $wait_for_executed_gtid_set=1
--source include/sync_replica_sql_with_primary.inc

# CLEANUP
--connection primary
DROP TABLE t1;
DROP USER `ABC`;
--source include/rpl_end.inc
