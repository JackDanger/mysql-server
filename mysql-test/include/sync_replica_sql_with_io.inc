# ==== Purpose ====
#
# Sync the replica SQL thread with the IO thread on the current connection.
#
# ==== Usage ====
#
# [--let $use_gtids= 1]
# [--let $rpl_channel_name= 1]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# --source include/sync_replica_sql_with_io.inc
#
# Parameters:
#
#   $use_gtids
#     If set, uses GTIDs instead of filename and offset for positions.
#
#   $rpl_channel_name
#     See include/wait_for_replica_param.inc
#
#   $replica_timeout
#     By default, the synchronization timeouts after 300 seconds. If
#     $replica_timeout is set, the synchronization timeouts after
#     $replica_timeout seconds.
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= sync_replica_sql_with_io.inc
--source include/begin_include_file.inc

--source include/save_io_thread_pos.inc
--source include/sync_replica_sql.inc

--let $include_filename= sync_replica_sql_with_io.inc
--source include/end_include_file.inc
