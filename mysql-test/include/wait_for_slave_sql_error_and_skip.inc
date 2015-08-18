# ==== Purpose ====
#
# Wait for replica SQL error, skip the erroneous statement and restart
# replica
#
# ==== Usage ====
#
# --let $replica_sql_error= NUMBER
# [--let $show_sql_error= 1]
# [--let $replica_skip_counter= 1]
# [--let $not_switch_connection= 1]
# [--let $rpl_channel_name= NAME]
# [--let $rpl_debug= 1]
# [--let $replica_timeout= NUMBER]
# --source include/wait_for_replica_sql_error_and_skip.inc
#
# Parameters:
#
#   $replica_sql_errno
#     The error number to wait for. This is required. (See
#     wait_for_replica_sql_error.inc)
#
#   $show_sql_error
#     If set, will print the error to the query log.
# 
#   $replica_skip_counter
#     If set, skip this number of events. If not set, skip one event.
#
#   $not_switch_connection
#     By default, this script executes on the connection 'replica'.  If
#     $not_switch_connection is set, this script executes on the current
#     connection.  In any case, the original connection is restored.
#
#   $rpl_channel_name
#     See include/wait_for_replica_param.inc
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= wait_for_replica_sql_error_and_skip.inc [errno=$replica_sql_errno]
--source include/begin_include_file.inc


if (!$rpl_debug)
{
  --disable_query_log
}


if (!$not_switch_connection)
{
  --let $rpl_connection_name= replica
  --source include/rpl_connection.inc
}


source include/wait_for_replica_sql_error.inc;


# skip the erroneous statement
if ($replica_skip_counter) {
  eval SET GLOBAL SQL_REPLICA_SKIP_COUNTER= $replica_skip_counter;
}
if (!$replica_skip_counter) {
  SET GLOBAL SQL_REPLICA_SKIP_COUNTER=1;
}
source include/start_replica.inc;


--let $include_filename= wait_for_replica_sql_error_and_skip.inc [errno=$replica_sql_errno]
--source include/end_include_file.inc
