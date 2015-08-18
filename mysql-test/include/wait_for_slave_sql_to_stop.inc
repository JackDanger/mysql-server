# ==== Purpose ====
#
# Waits until the SQL thread of the current connection has stopped
# gracefully.
#
# Note: this script will fail if the SQL thread stops with an error.
# If you expect an error in the SQL thread, use
# include/wait_for_replica_io_error.inc instead.
#
# This script also fails if a timeout is reached (default 300
# seconds).
#
#
# ==== Usage ====
#
# [--let $rpl_channel_name= NAME]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# --source include/wait_for_replica_sql_to_stop.inc
#
# Parameters:
#   $rpl_channel_name
#     See include/wait_for_replica_param.inc
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= wait_for_replica_sql_to_stop.inc
--source include/begin_include_file.inc


--let $replica_param= Replica_SQL_Running
--let $replica_param_value= No
--let $replica_param_comparison= =
--let $replica_error_param= Last_SQL_Errno
--source include/wait_for_replica_param.inc
--let $replica_error_param= 


--let $include_filename= wait_for_replica_sql_to_stop.inc
--source include/end_include_file.inc
