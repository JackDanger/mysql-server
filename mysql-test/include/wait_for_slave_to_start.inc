# ==== Purpose ====
#
# Waits until both the IO and SQL threads of the current connection
# have started, or until a timeout is reached.
#
#
# ==== Usage ====
#
# [--let $rpl_channel_name= NAME]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# --source include/wait_for_replica_to_start.inc
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


--let $include_filename= wait_for_replica_to_start.inc
--source include/begin_include_file.inc


--source include/wait_for_replica_io_to_start.inc
--source include/wait_for_replica_sql_to_start.inc


--let $include_filename= wait_for_replica_to_start.inc
--source include/end_include_file.inc
