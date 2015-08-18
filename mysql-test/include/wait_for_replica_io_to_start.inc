# ==== Purpose ====
#
# Waits until the IO thread of the current connection has started and
# connected to the primary (i.e., until SHOW REPLICA STATUS returns Yes
# in the Replica_IO_Running field), or until a timeout is reached.
#
#
# ==== Usage ====
#
# [--let $rpl_channel_name= NAME]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# [--let $rpl_allow_error= 1]
# --source include/wait_for_replica_io_to_start.inc
#
# Parameters:
#   $rpl_channel_name
#     See include/wait_for_replica_param.inc
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
#
#   $rpl_allow_error
#     By default, this file fails if there is an error in the IO
#     thread.  However, the IO thread can recover and reconnect after
#     certain errors.  If such an error is expected, can set
#     $rpl_allow_error=1.  This will prevent this file from failing if
#     there is an error in the IO thread.
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= wait_for_replica_io_to_start.inc
--source include/begin_include_file.inc


let $replica_param= Replica_IO_Running;
let $replica_param_value= Yes;
if (!$rpl_allow_error)
{
  --let $replica_error_param= Last_IO_Errno
}
source include/wait_for_replica_param.inc;
--let $replica_error_param=
--let $rpl_allow_error= 0


--let $include_filename= wait_for_replica_io_to_start.inc
--source include/end_include_file.inc
