# ==== Purpose ====
#
# Assert that a given column in SHOW REPLICA STATUS has a given value.
#
# ==== Usage ====
#
# --let $replica_param= COLUMN_NAME
# --let $replica_param_value= VALUE
# [--let $rpl_channel_name = NAME]
# [--let $rpl_debug= 1]
# --source include/check_replica_param.inc
#
# Parameters:
#   $replica_param, $replica_param_value
#     Column name in output of SHOW REPLICA STATUS that should be checked,
#     and the expected value. Example:
#       --let $replica_param= Exec_Primary_Log_Pos
#       --let $replica_param_value= 4711
#
#   $rpl_channel_name
#     If set, add a FOR CHANNEL '$rpl_channel_name' clause.
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $_param= $replica_param

--let $_csp_for_channel=
if ($rpl_channel_name != '')
{
  --let $_csp_for_channel= FOR CHANNEL '$rpl_channel_name'
  --let $param= $_param, channel=$rpl_channel_name
}


--let $include_filename= check_replica_param.inc [$_param]
--source include/begin_include_file.inc


--let $_param_value= query_get_value(SHOW REPLICA STATUS $_csp_for_channel, $replica_param, 1)
if ($_param_value != $replica_param_value) {
  --source include/show_rpl_debug_info.inc
  --echo Wrong value for $replica_param. Expected '$replica_param_value', got '$_param_value'
  --die Wrong value for replica parameter
}


--let $include_filename= check_replica_param.inc
--source include/end_include_file.inc
