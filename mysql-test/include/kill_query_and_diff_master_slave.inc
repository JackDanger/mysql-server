# ==== Purpose ====
#
# Kill a query, sync primary with replica, and diff the output of a
# statement on primary and replica to check if statement is correctly
# replicated.
#
# ==== Usage ====
#
# connection <CONNECTION>;
# let $connection_name=<CONNECTION>
# let $connection_id=`SELECT CONNECTION_ID()`;
# let $rpl_diff_statement=<SQL COMMAND>;
# send <SQL COMMAND>;
# source include/kill_query_and_diff_primary_replica.inc;
#
# Note: <CONNECTION> must not be 'primary'.
#
# See also kill_query.inc and rpl_diff.inc for more
# information

source include/kill_query.inc;

# Release the debug lock if used, so that the statements in
# rpl_diff.inc will not be blocked.
connection primary;
disable_query_log;
disable_result_log;
if ($debug_lock)
{
  eval SELECT RELEASE_LOCK($debug_lock);
}
enable_result_log;
enable_query_log;

--source include/rpl_diff.inc

# Acquire the debug lock again if used
connection primary;
disable_query_log; disable_result_log;
if ($debug_lock) { eval SELECT GET_LOCK($debug_lock, 10); }
enable_result_log; enable_query_log;

connection $connection_name;
