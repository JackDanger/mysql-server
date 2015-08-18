# ==== Purpose ====
#
# Waits the SQL thread of the current connection has started, or until
# a timeout is reached.
#
# ==== Usage ====
#
# [--let $rpl_channel_name= NAME]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# --source include/wait_for_replica_sql_to_start.inc
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


--let $include_filename= wait_for_replica_sql_to_start.inc
--source include/begin_include_file.inc


let $replica_param= Replica_SQL_Running;
let $replica_param_value= Yes;

# Unfortunately, the replica sql thread sets Replica_SQL_Running=Yes
# *before* it clears Last_SQL_Errno.  So we have to allow errors in
# the SQL thread here.

#--let $replica_error_param= Last_SQL_Errno
source include/wait_for_replica_param.inc;
#--let $replica_error_param=


--let $include_filename= wait_for_replica_sql_to_start.inc
--source include/end_include_file.inc
