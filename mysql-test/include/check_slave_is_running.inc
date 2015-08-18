# ==== Purpose ====
#
# Assert that the replica threads are running and don't have any errors.
#
#
# ==== Usage ====
#
# [--let $rpl_channel_name = NAME]
# [--let $rpl_debug= 1]
# --source include/check_replica_is_running.inc
#
# Parameters:
#   $rpl_channel_name
#     See include/check_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= check_replica_is_running.inc
--source include/begin_include_file.inc


--let $replica_param= Replica_IO_Running
--let $replica_param_value= Yes
--source include/check_replica_param.inc

--let $replica_param= Replica_SQL_Running
--let $replica_param_value= Yes
--source include/check_replica_param.inc


--let $include_filename= check_replica_is_running.inc
--source include/end_include_file.inc
