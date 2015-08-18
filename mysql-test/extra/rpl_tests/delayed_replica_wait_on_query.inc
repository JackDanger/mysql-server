# ==== Purpose ====
#
# Auxiliary file used by rpl_delayed_replica.test.  This assumes that an
# 'INSERT INTO t1...' query has been executed on the primary.  It does
# this:
#
# - After half the delay, check the status. It should be delaying and
#   the query should not have executed.
#
# - After one and a half delay, check the status. It should not be
#   delaying and the query should be executed.
#
#
# ==== Usage ====
#
# --let $query_number= 4
# --source extra/rpl_tests/delayed_replica_wait_on_query.inc
#
# Parameters:
# $query_number
#   The value of the 'b' column in t1 for the row inserted by the query
#   we are waiting for.

connection primary;

--echo [on replica]
--let $replica_timeout= $time1
--source include/sync_replica_io_with_primary.inc
--echo # sleep 1*T
--sleep $time1

--let $assert_text= Query $query_number should not be executed
--let $assert_cond= MAX(b) < $query_number FROM t1
--source include/assert.inc

--let $assert_text= Status should be 'Waiting until PRIMARY_DELAY...'
--let $assert_cond= "[SHOW REPLICA STATUS, Replica_SQL_Running_State, 1]" LIKE "Waiting until PRIMARY_DELAY%"
--source include/assert.inc

--echo # sleep 1*T
--sleep $time1

--echo # sync with primary (with timeout 1*T)
--source include/sync_replica_sql.inc

--let $assert_text= Query $query_number should be executed
--let $assert_cond= MAX(b) = $query_number FROM t1
--source include/assert.inc

--let $assert_text= Status should be 'Has read all relay log...'
--let $assert_cond= "[SHOW REPLICA STATUS, Replica_SQL_Running_State, 1]" LIKE "Replica has read all relay log%"
--source include/assert.inc


--source include/check_replica_is_running.inc
