# ==== Purpose ====
#
# Assert that Replica_SQL_Errno = Replica_IO_Errno = 0 in the output from
# SHOW REPLICA STATUS.
#
#
# ==== Usage ====
#
# [--let $rpl_channel_name = NAME]
# [--let $rpl_debug= 1]
# --source include/check_replica_no_error.inc
#
# Parameters:
#   $rpl_channel_name
#     See include/check_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= check_replica_no_error.inc
--source include/begin_include_file.inc


--let $replica_param= Last_SQL_Errno
--let $replica_param_value= 0
--source include/check_replica_param.inc

--let $replica_param= Last_IO_Errno
--let $replica_param_value= 0
--source include/check_replica_param.inc


--let $include_filename= check_replica_no_error.inc
--source include/end_include_file.inc
