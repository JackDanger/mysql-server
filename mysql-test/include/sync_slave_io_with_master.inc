# ==== Purpose ====
#
# Waits until the replica IO thread has been synced, i.e., all events
# have been copied over to replica.  Does not care if the SQL thread is
# in sync (or even running).
#
#
# ==== Usage ====
#
# [--let $sync_replica_connection= <connection_name>]
# [--let $use_gtids= 1]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# --source include/sync_replica_io_with_primary.inc
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
#   $replica_timeout
#      See include/wait_for_replica_param.inc.
#
#   $rpl_debug
#      See include/rpl_init.inc

--let $include_filename= sync_replica_io_with_primary.inc
--source include/begin_include_file.inc


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

--source include/sync_replica_io.inc


--let $include_filename= sync_replica_io_with_primary.inc
--let $skip_restore_connection= 1
--source include/end_include_file.inc
