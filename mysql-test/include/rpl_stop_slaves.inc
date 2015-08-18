# ==== Purpose ====
#
# Stop all replicas configured by rpl_init.inc and waits for the replica
# threads to stop.
#
#
# ==== Usage ====
#
# [--let $rpl_only_running_threads= 1]
# [--let $rpl_debug= 1]
# [--let $replica_timeout= NUMBER]
# --source include/rpl_stop_replicas.inc
#
# Parameters:
#   $rpl_only_running_threads
#     See include/stop_replica.inc
#
#   $replica_timeout
#     Set the timeout when waiting for replica threads to stop.  See
#     include/wait_for_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= rpl_stop_replicas.inc
--source include/begin_include_file.inc

--let $rpl_source_file= include/stop_replica.inc
--source include/rpl_for_each_connection.inc

--let $include_filename= rpl_stop_replicas.inc
--source include/end_include_file.inc
