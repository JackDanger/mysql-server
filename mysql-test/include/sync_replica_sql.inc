# ==== Purpose ====
#
# This file does the same as the built-in command sync_with_primary,
# but more flexible. In particular:
#  - Can use a custom timeout.
#  - Can use WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS instead of PRIMARY_POS_WAIT.
#  - The position can be set by the user, it is not restricted to
#    save_primary_pos
#
# ==== Usage ====
#
# --connection primary
# --source include/save_primary_pos.inc
# --connection replica
# [--let $replica_timeout= X]
# [--let $use_gtids= 1]
# [--let $wait_for_executed_gtid_set= 1]
# [--let $ignore_gtids_on_sync= 1]
# --source include/sync_replica.inc
#
# Parameters:
#   $replica_timeout
#     See wait_for_replica_param.inc
#
#   $use_gtids
#     If set, uses GTIDs instead of filename and offset for positions.
#
#   $ignore_gtids_on_sync
#     Forces the use of primary file and position, even if $use_gtids is set.
#     This might be used if the replica will not have all the GTIDs of the primary
#     but have to read and apply all primary events to the end.
#
#   $wait_for_executed_gtid_set
#     See include/sync_replica_sql_with_primary.inc

--let $include_filename= sync_replica_sql.inc
--source include/begin_include_file.inc

--let $_sss_replica_timeout= $replica_timeout
if (!$_sss_replica_timeout)
{
  --let $_sss_replica_timeout= 300
}

--let $_sss_channel_param=
if ($_saved_channel_name != '')
{
  --let $_sss_channel_param= , '$_saved_channel_name'
}

if ($rpl_debug)
{
  --echo use_gtids='$use_gtids' ignore_gtids_on_sync='$ignore_gtids_on_sync' _saved_gtids='$_saved_gtids' _saved_file='$_saved_file' _saved_pos='$_saved_pos' timeout='$_sss_replica_timeout'
}

#
# Verify if the SQL thread is already updated by the primary position
# regardless the use if GTIDs. If the SQL thread is already updated,
# we can reduce the replica timeout of WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS
# to a minimum because there in nothing else to sync.
# As there is a possibility of missing GTIDs on replica, this reduced
# timeout will make mtr throw an error almost promptly, without having
# to wait 300 seconds to notice a sync problem between primary and replica.
#
--let $_replica_primary_file= query_get_value(SHOW REPLICA STATUS, Relay_Primary_Log_File, 1)
--let $_replica_primary_pos= query_get_value(SHOW REPLICA STATUS, Exec_Primary_Log_Pos, 1)
if ($_replica_primary_file == $_saved_file)
{
  if ($_replica_primary_pos == $_saved_pos)
  {
    if ($rpl_debug)
    {
      --echo debug: It seems that SQL thread is already synced (by verifying primary file and position)
      --echo debug: Changing replica timeout to 1 seconds
    }
    --let $_sss_replica_timeout= 1
  }
}

--let $_sync_with_gtids= $use_gtids
if ($ignore_gtids_on_sync)
{
  --let $_sync_with_gtids= 0
}

if ($_sync_with_gtids)
{
  --source include/assert_gtid_mode_on.inc
  if ($wait_for_executed_gtid_set)
  {
    if ($rpl_debug)
    {
      --echo debug: WAIT_FOR_EXECUTED_GTID_SET('$_saved_gtids', $_sss_replica_timeout)
    }
    --let $_sss_result= `SELECT -WAIT_FOR_EXECUTED_GTID_SET('$_saved_gtids', $_sss_replica_timeout)`
  }
  if (!$wait_for_executed_gtid_set)
  {
    if ($rpl_debug)
    {
      --echo debug: WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS('$_saved_gtids', $_sss_replica_timeout$_sss_channel_param)
    }
    --let $_sss_result= `SELECT WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS('$_saved_gtids', $_sss_replica_timeout$_sss_channel_param)`
  }
}

if (!$_sync_with_gtids)
{
  if ($rpl_debug)
  {
    --echo debug: PRIMARY_POS_WAIT('$_saved_file', $_saved_pos, $_sss_replica_timeout,$_sss_channel_param)
  }
  --let $_sss_result= `SELECT PRIMARY_POS_WAIT('$_saved_file', $_saved_pos, $_sss_replica_timeout$_sss_channel_param)`
}

if ($_sss_result == '')
{
  --let $_sss_result = -2
  --let $error_type= The replica SQL thread was not started, the replica's primary information was not initialized, the arguments were incorrect, or an error occurred.
}
if ($_sss_result == -1)
{
  --let $error_type= Timeout after $_sss_replica_timeout seconds.
}

if ($_sss_result < 0)
{
  --source include/show_rpl_debug_info.inc
  --echo ERROR: sync_replica_sql.inc failed on connection '$CURRENT_CONNECTION'
  --echo ERROR: use_gtids='$use_gtids'
  --echo ERROR: _saved_gtids='$_saved_gtids'
  --echo ERROR: _saved_file='$_saved_file'
  --echo ERROR: _saved_pos='$_saved_pos'
  --echo ERROR: timeout='$_sss_replica_timeout'
  --echo ERROR: result='$_sss_result'
  --echo ERROR: error type: $error_type
  --die Error in sync_with_primary.inc
}

--let $include_filename= sync_replica_sql.inc
--source include/end_include_file.inc
