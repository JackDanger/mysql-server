# ==== Purpose ====
#
# Waits until the IO thread of the current connection has got an
# error, or until a timeout is reached. Also waits until the IO
# thread has completely stopped.
#
#
# ==== Usage ====
#
# --let $replica_io_errno= NUMBER [, NUMBER ...] [# comment]
# [--let $show_replica_io_error= 1]
# [--let $replica_io_error_is_nonfatal= 1]
# [--let $replica_io_error_replace= REGEX]
# [--let $rpl_channel_name= NAME]
# [--let $rpl_debug= 1]
# [--let $replica_timeout= NUMBER]
# --source include/wait_for_replica_io_error.inc
#
# Parameters:
#   $replica_io_errno
#     The expected IO error numbers. This can be either a single
#     number, or a comma-separated list of numbers. Examples:
#       --let $replica_io_errno= 1040, 1053, 2002, 2003, 2006, 2013
#       --let $replica_io_errno= 1593 # ER_REPLICA_FATAL_ERROR
#     It is allowed to add a comment at the end of the line, and it
#     is strongly advised to put the symbolic name of the error in
#     this comment, like the example above.
#     (After BUG#41956 has been fixed, this will be required to be
#     symbolic names instead of numbers.)
#
#   $show_replica_io_error
#     If set, will print the error to the query log.
#
#   $replica_io_error_is_nonfatal
#     By default, we wait for the replica IO thread to stop completely
#     (i.e., until Replica_IO_State is empty).  If this variable is set,
#     then we don't wait. This is useful if the error is non-fatal
#     (e.g., temporary connection error) and does not cause the replica
#     IO thread to stop.
# 
#   $replica_io_error_replace
#     If set, one or more regex patterns for replacing variable
#     text in the error message. Syntax as --replace-regex
#
#   $rpl_channel_name
#     See include/wait_for_replica_param.inc.
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
# 
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= wait_for_replica_io_error.inc [errno=$replica_io_errno]
--source include/begin_include_file.inc


let $old_replica_param_comparison= $replica_param_comparison;
let $replica_param= Last_IO_Errno;
let $replica_param_comparison= !=;
let $replica_param_value= 0;
source include/wait_for_replica_param.inc;
let $replica_param_comparison= $old_replica_param_comparison;

let $_wait_for_replica_io_errno= query_get_value($replica_param_statement, Last_IO_Errno, 1);

if ($replica_io_errno == '')
{
  --echo !!!ERROR IN TEST: you must set \$replica_io_errno before you source
  --echo !!!wait_for_replica_io_error.inc. The error we got this time was '$_wait_for_replica_io_errno',
  --echo !!!so you probably want to add the following line to your test case:
  --echo !!!  --let \$replica_io_errno= $_wait_for_replica_io_errno
  --vertical_results
  eval $replica_param_statement;
  --die !!!ERROR IN TEST: you must set \$replica_io_errno before sourcing wait_for_replica_io_error.inc
}

# Strip away comments
--let $_replica_io_errno= `SELECT IF(LOCATE('#', '$replica_io_errno') != 0, SUBSTR('$replica_io_errno', 1, LOCATE('#', '$replica_io_errno') - 1), '$replica_io_errno')`
if ($rpl_debug)
{
  --echo # debug: replica_io_errno='$replica_io_errno'; stripped from comments='$_replica_io_errno' wait_for_replica_io_errno='$_wait_for_replica_io_errno'
}

if (`SELECT $_wait_for_replica_io_errno NOT IN ($_replica_io_errno)`)
{
  --echo **** Replica stopped with wrong error code: $_wait_for_replica_io_errno (expected $replica_io_errno) ****
  source include/show_rpl_debug_info.inc;
  --echo **** Replica stopped with wrong error code: $_wait_for_replica_io_errno (expected $replica_io_errno) ****
  --die Replica stopped with wrong error code
}

if ($show_replica_io_error)
{
  --let $_wait_for_replica_io_error_old_status_items= $status_items
  --let $status_items= Last_IO_Error
  --let $replica_field_result_replace= $replica_io_error_replace
  --source include/show_replica_status.inc
  --let $status_items= $_wait_for_replica_io_error_old_status_items
}

if (!$replica_io_error_is_nonfatal)
{
  --let $replica_param= Replica_IO_State
  --let $replica_param_value=
  --source include/wait_for_replica_param.inc
}
--let $replica_io_error_is_nonfatal= 0


--let $include_filename= wait_for_replica_io_error.inc [errno=$replica_io_errno]
--source include/end_include_file.inc
