# ==== Purpose ====
#
# Start all replicas configured by rpl_init.inc and wait for the replica
# threads to start.
#
# Note that rpl_init.inc calls this file automatically, so you only
# need to source this file if the replicas have stopped after that.
#
#
# ==== Usage ====
#
# [--let $rpl_debug= 1]
# [--let $replica_timeout= NUMBER]
# --source include/rpl_start_replicas.inc
#
# Parameters:
#   $rpl_debug
#     See include/rpl_init.inc
#
#   $replica_timeout
#     Set the timeout when waiting for replica threads to stop and
#     start, respectively.  See include/wait_for_replica_param.inc


--let $include_filename= rpl_start_replicas.inc
--source include/begin_include_file.inc

--let $rpl_source_file= include/start_replica.inc
--source include/rpl_for_each_connection.inc

--let $include_filename= rpl_start_replicas.inc
--source include/end_include_file.inc
