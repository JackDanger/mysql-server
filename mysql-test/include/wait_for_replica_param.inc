# ==== Purpose ====
#
# Waits until SHOW REPLICA STATUS has returned a specified value, or
# until a timeout is reached.
#
#
# ==== Usage ====
#
# --let $replica_param= Replica_SQL_Running
# --let $replica_param_value= No
# [--let $replica_param_comparison= [ < | <= | >= | > | = | != ]]
# [--let $replica_timeout= NUMBER]
# [--let $replica_error_param= [Replica_SQL_Errno | Replica_IO_Errno]]
# [--let $rpl_channel_name= NAME]
# [--let $rpl_debug= 1]
# --source include/wait_for_replica_param.inc
#
# Parameters:
#
# $replica_param, $replica_param_value
#   This macro will wait until the column of the output of SHOW REPLICA
#   STATUS named $replica_param gets the value $replica_param_value.  See
#   the example above.
#
# $replica_param_comparison
#   By default, this file waits until $replica_param becomes equal to
#   $replica_param_value.  If you want to wait until $replica_param
#   becomes *unequal* to $replica_param_value, set this parameter to the
#   string '!=', like this:
#     --let $replica_param_comparison= !=
#
# $rpl_channel_name
#   If this is not '', adds a FOR CHANNEL  '$rpl_channel_name' clause
#   to SHOW REPLICA STATUS.
#
# $replica_timeout
#   The default timeout is 30 seconds. You can change the timeout by
#   setting $replica_timeout. The unit is one second.
#
# $replica_error_param
#   If set, this script will check if the column of the output from
#   SHOW REPLICA STATUS named $replica_error_param is nonzero.  If it is,
#   this script will faile immediately.  Typically, this should be set
#   to Last_IO_Errno or Last_SQL_Errno.
#
# $rpl_debug
#   See include/rpl_init.inc
#
# ==== mysqltest variables configured by this file ====
#
# This file sets $replica_param_statement to the SQL statement used to
# get the replica status: either SHOW REPLICA STATUS or SHOW REPLICA STATUS
# FOR CHANNEL  '<channel>'.


--let $_wait_for_replica_params= $replica_param
if ($rpl_channel_name != '')
{
  --let $_wait_for_replica_params= $_wait_for_replica_params channel=$rpl_channel_name
}
--let $include_filename= wait_for_replica_param.inc [$_wait_for_replica_params]
--source include/begin_include_file.inc

--let $default_timeout= 30

--let $sleep_freq= 10
--let $sleep_time= `select 1.0 / $sleep_freq`

--let $start_to_wait=`select current_timestamp()`

let $_replica_timeout= $replica_timeout;
if (!$_replica_timeout)
{
  let $_replica_timeout= `select $default_timeout * $sleep_freq`;
}
if ($VALGRIND_TEST) {
  let $_replica_timeout= `select $_replica_timeout * 6`;
}

if ($replica_error_param == '')
{
  --let $replica_error_param= 1
}

let $_replica_param_comparison= $replica_param_comparison;
if (!$_replica_param_comparison)
{
  let $_replica_param_comparison= =;
}

if ($rpl_debug)
{
  --echo Waiting until '$replica_param' $_replica_param_comparison '$replica_param_value' [timeout='$_replica_timeout', \$replica_error_param='$replica_error_param']
}

--let $replica_param_statement= SHOW REPLICA STATUS
if ($rpl_channel_name != '')
{
  --let $replica_param_statement= $replica_param_statement FOR CHANNEL  '$rpl_channel_name'
}

--let $_replica_check_configured= query_get_value($replica_param_statement, Replica_IO_Running, 1)

if ($_replica_check_configured == 'No such row')
{
  --echo **** ERROR: $replica_param_statement returned empty result set. Replica not configured. ****
  --source include/show_rpl_debug_info.inc
  --die SHOW REPLICA STATUS returned empty result set. Replica not configured.
}

--let $_replica_timeout_counter= `select $_replica_timeout * $sleep_freq`
--let $_replica_continue= 1
while ($_replica_continue)
{
  --let $_show_replica_status_value= query_get_value($replica_param_statement, $replica_param, 1)

  # Check if an error condition is reached.
  if (!$replica_error_param)
  {
    --let $_show_replica_status_error_value= query_get_value($replica_param_statement, $replica_error_param, 1)
    if ($_show_replica_status_error_value)
    {
      --echo **** ERROR: $replica_error_param = '$_show_replica_status_error_value' while waiting for replica parameter $replica_param $_replica_param_comparison $replica_param_value ****
      --source include/show_rpl_debug_info.inc
      --die Error condition reached in include/wait_for_replica_param.inc
    }
  }

  # Check if the termination condition is reached.
  --let $_replica_continue= `SELECT NOT('$_show_replica_status_value' $_replica_param_comparison '$replica_param_value')`

  # Decrease timer, and check if the timeout is reached.
  if ($_replica_continue)
  {
    --dec $_replica_timeout_counter
    if (!$_replica_timeout_counter)
    {
      --let $end_to_wait=`select current_timestamp()`

      --echo **** ERROR: timeout after $_replica_timeout ($end_to_wait - $start_to_wait) seconds while waiting for replica parameter $replica_param $_replica_param_comparison $replica_param_value ****
      --source include/show_rpl_debug_info.inc
      --die Timeout in include/wait_for_replica_param.inc
    }
    --sleep  $sleep_time
  }
}


--let $include_filename= wait_for_replica_param.inc [$replica_param]
--source include/end_include_file.inc
