# WL#7796 - WAIT_FOR_EXECUTED_GTID_SET
#
# This worklog aims at introducing a new function which waits for the
# gtid_executed set to be in sync with the given gtid_set with option
# of allowing a specified timeout duration or without timeout duration
# The new wait function is generic one and it doen not depend on the
# replica info to do the waiting.
#
# This test checks for the positive functionality of the new
# wait_for_executed_gtid_set for a given gtid set and also checks for the
# timeout condition if the given gtid is not generated.
#
# To run this test you need to define:
#  $gtid_wait_set              The set used to check the server is waiting
#  $gtid_wait_set_for_timeout  The set used to verify the function timeouts
#

if ($gtid_wait_set == '')
{
  --die ERROR IN TEST: the "gtid_wait_set" variable must be set
}

if ($gtid_wait_set_for_timeout == '')
{
  --die ERROR IN TEST: the "gtid_wait_set_for_timeout" variable must be set
}

--let $rpl_gtid_utils= 1
--source include/have_log_bin.inc
--source include/have_gtid.inc
--source include/primary-replica.inc

--connection replica
--let $primary_uuid= query_get_value(select SOURCE_UUID from performance_schema.replication_connection_status WHERE service_state = 'ON', SOURCE_UUID, 1)
--connection primary

--let $gtid_wait_set= $primary_uuid:$gtid_wait_set
--let $gtid_wait_set_for_timeout= $primary_uuid:$gtid_wait_set_for_timeout

--connection primary
CREATE TABLE t1 (c1 INT NOT NULL PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO t1 VALUES (1);

--connection primary1

--let $connection_id= `SELECT CONNECTION_ID()`
--send
--replace_result $primary_uuid PRIMARY_UUID
--eval SELECT WAIT_FOR_EXECUTED_GTID_SET('$gtid_wait_set', 10000)

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

--echo ###############################
--echo Should succeed without timeout
--replace_result $primary_uuid PRIMARY_UUID
--let $result= `SELECT WAIT_FOR_EXECUTED_GTID_SET('$gtid_wait_set', 1)`
--let $assert_text= The value returned by the wait for executed gtid function should be 0
--let $assert_cond= $result = 0
--source include/assert.inc

--echo

--echo ######################
--echo Timeout should happen
--replace_result $primary_uuid PRIMARY_UUID
--let $result= `SELECT WAIT_FOR_EXECUTED_GTID_SET('$gtid_wait_set_for_timeout', 1)`
--let $assert_text= The value returned by the wait for executed gtid function should be 1 indicating a timeout
--let $assert_cond= $result = 1
--source include/assert.inc

# The wait function should return if its waiting for a gtid set which is
# already committed.

--connection primary1

--replace_result $primary_uuid PRIMARY_UUID
--eval SELECT WAIT_FOR_EXECUTED_GTID_SET('', 9999)

--replace_result $primary_uuid PRIMARY_UUID
--eval SELECT WAIT_FOR_EXECUTED_GTID_SET('')

--replace_result $primary_uuid PRIMARY_UUID
--eval SELECT WAIT_FOR_EXECUTED_GTID_SET('$primary_uuid:1-2', 9999)

--let $wait_for_executed_gtid_set=1

# CLEANUP
--connection primary
DROP TABLE t1;
--source include/rpl_end.inc
