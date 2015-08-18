# ==== Purpose ====
#
# Waits until the replica SQL thread has been synced, i.e., all events
# have been copied over to replica.  This is like mtr's built-in command
# sync_replica_with_primary, but more flexible (e.g., you can set a
# custom timeout and you can force it to use GTIDs instead of filename
# and offset).
#
#
# ==== Usage ====
#
# [--let $sync_replica_connection= <connection_name>]
# [--let $use_gtids= 1]
# [--let $rpl_channel_name= 1]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# [--let $wait_for_executed_gtid_set= 1]
# [--let $ignore_gtids_on_sync= 1]
# --source include/sync_replica_sql_with_primary.inc
#
# Must be called on the primary. Will change connection to the replica.
#
# Parameters:
#
#   $use_gtids
#     If set, uses GTIDs instead of filename and offset for positions.
#
#   $sync_replica_connection
#     By default, this script switches connection to 'replica'. If
#     $sync_replica_connection is set, then '$sync_replica_connection' is
#     used instead of 'replica'.
#
#   $rpl_channel_name
#     See include/wait_for_replica_param.inc
#
#   $replica_timeout
#      See include/wait_for_replica_param.inc.
#
#   $rpl_debug
#      See include/rpl_init.inc
#
#   $ignore_gtids_on_sync
#     Forces the use of primary file and position, even if $use_gtids is set.
#     This might be used if the replica will not have all the GTIDs of the
#     primary but have to read and apply all primary events to the end.
#
#   $wait_for_executed_gtid_set
#      This option makes the test use WAIT_FOR_EXECUTED_GTID_SET instead of
#      WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS. This options also requires
#      $use_gtids and !$ignore_gtids_on_sync to be set.
#

--let $include_filename= sync_replica_sql_with_primary.inc
--source include/begin_include_file.inc

--source include/wait_for_ndb_committed_to_binlog.inc
--source include/save_primary_pos.inc

--let $rpl_connection_name= replica
if ($sync_replica_connection)
{
  --let $rpl_connection_name= $sync_replica_connection
}

# Assert that source and destination connections are not the same.
if ($rpl_connection_name == $CURRENT_CONNECTION)
{
  --echo Trying to sync '$CURRENT_CONNECTION' with '$rpl_connection_name' which are the same server!
  --echo Please double check your test case.
  --die
}
--source include/rpl_connection.inc

--source include/sync_replica_sql.inc

--let $include_filename= sync_replica_sql_with_primary.inc
--let $skip_restore_connection= 1
--source include/end_include_file.inc
