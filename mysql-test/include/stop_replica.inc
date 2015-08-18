# ==== Purpose ====
#
# Issues STOP REPLICA on the current connection. Then waits until both
# the IO and SQL threads have stopped, or until a timeout is reached.
#
# Please use this instead of 'STOP REPLICA', to reduce the risk of races
# in test cases.
#
# This will fail if the replica IO or SQL thread has an error. If you
# expect an error in the IO thread, use
# include/wait_for_replica_io_error.inc and include/stop_replica_sql.inc.
#
#
# ==== Usage ====
#
# [--let $rpl_only_running_threads= 1]
# [--let $rpl_channel_name= NAME]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# --source include/stop_replica.inc
#
# Parameters:
#   $rpl_only_running_threads
#     By default, this script executes STOP REPLICA unconditionally.
#     This generates a warnings if one or both replica threads are
#     already stopped.  If $rpl_only_running_threads is set, this
#     script checks which replica threads are running, and issues either
#     STOP REPLICA, STOP REPLICA SQL_THREAD, STOP REPLICA IO_THREAD, or
#     nothing.
#
#   $rpl_channel_name
#     If this is not '', a FOR CHANNEL  '$rpl_channel_name' is added.
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= stop_replica.inc
--source include/begin_include_file.inc


if (!$rpl_debug)
{
  --disable_query_log
}

if ($rpl_channel_name != '')
{
  --let $_for_channel_clause= FOR CHANNEL  '$rpl_channel_name'
}


if ($rpl_only_running_threads)
{
  --let $_replica_sql_running= query_get_value(SHOW REPLICA STATUS $_for_channel_clause, Replica_SQL_Running, 1)
  --let $_replica_io_running= query_get_value(SHOW REPLICA STATUS $_for_channel_clause, Replica_IO_Running, 1)
  if ($rpl_debug)
  {
    --echo Stop SQL: $_replica_sql_running; Stop IO: $_replica_io_running
  }

  --let $_replica_running_bits= `SELECT IF('$_replica_io_running' = 'Yes', 1, 0) + IF('$_replica_sql_running' = 'Yes', 2, 0)`
  if ($_replica_running_bits)
  {
    --dec $_replica_running_bits
    # $_replica_running_bits=1: io thread running
    if (!$_replica_running_bits)
    {
      --source include/stop_replica_io.inc
    }
    --dec $_replica_running_bits
    # $_replica_running_bits=2: sql thread running
    if (!$_replica_running_bits)
    {
      --source include/stop_replica_sql.inc
    }
    --dec $_replica_running_bits
    # $_replica_running_bits=2: both threads running
    if (!$_replica_running_bits)
    {
      eval STOP REPLICA $_for_channel_clause;
      --source include/wait_for_replica_to_stop.inc
    }
  }
}
if (!$rpl_only_running_threads)
{
  eval STOP REPLICA $_for_channel_clause;
  --source include/wait_for_replica_to_stop.inc
}


--let $include_filename= stop_replica.inc
--source include/end_include_file.inc
