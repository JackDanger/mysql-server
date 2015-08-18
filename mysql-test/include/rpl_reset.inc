# ==== Purpose ====
#
# Reset all replication servers to a clean state:
#
#  - sync all replicas,
#  - stop all replicas (STOP REPLICA),
#  - remove all binlogs and relay logs (RESET PRIMARY and RESET REPLICA),
#  - start all replicas again (START REPLICA).
#
# It does not execute CHANGE PRIMARY, so the replication topology is
# kept intact.
#
#
# ==== Usage ====
#
# [--let $rpl_only_running_threads= 1]
# [--let $rpl_debug= 1]
# [--let $replica_timeout= NUMBER]
# --source include/rpl_end.inc
#
# Parameters:
#   $rpl_only_running_threads
#     If one or both of the IO and SQL threads is stopped, sync and
#     stop only the threads that are running.  See
#     include/rpl_sync.inc and include/stop_replica.inc for details.
#
#   $rpl_skip_sync
#     By default, all replicas are synced using rpl_sync.inc. Set this
#     option to 1 to disable this behavior (note that you must
#     manually sync all servers in this case). Normally you want to
#     sync, but you need to disable sync if you use multi-source.
#
#   $rpl_debug
#     See include/rpl_init.inc
#
#   $rpl_no_start_replica
#     Don't execute START REPLICA.
#
#   $replica_timeout
#     Set the timeout when waiting for replica threads to stop and
#     start, respectively.  See include/wait_for_replica_param.inc
#
# Note:
#   This script will fail if Last_SQL_Error or Last_IO_Error is
#   nonempty.  If you expect an error in the SQL thread, you should
#   normally do this before you source include/rpl_reset.inc:
#
#     --source include/wait_for_replica_sql_error.inc
#     --source include/stop_replica_io.inc
#     RESET REPLICA;

--let $include_filename= rpl_reset.inc
--source include/begin_include_file.inc

if (!$rpl_debug)
{
  --disable_query_log
}

# Sync
if (!$rpl_skip_sync)
{
  --source include/rpl_sync.inc
}

# STOP REPLICA
--source include/rpl_stop_replicas.inc

# RESET REPLICA
--let $rpl_source_file= include/rpl_reset_replica_helper.inc
--source include/rpl_for_each_connection.inc

# RESET PRIMARY
--let $rpl_source_file= include/rpl_reset_primary_helper.inc
--source include/rpl_for_each_server.inc

# START REPLICA
if (!$rpl_no_start_replica)
{
  --source include/rpl_start_replicas.inc
}


--let $include_filename= rpl_reset.inc
--source include/end_include_file.inc
