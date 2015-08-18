# ==== Purpose ====
#
# Waits until the replica IO thread on the current connection has been
# synced up to the point saved by the last call to
# include/save_primary_pos.inc (i.e., until the IO thead has copied up
# to the saved position).  Does not wait for the SQL thread.
#
#
# ==== Usage ====
#
# [--let $use_gtids= 1]
# [--let $rpl_debug= 1]
# [--let $replica_timeout= NUMBER]
#
# On primary:
# --source include/save_primary_pos.inc
#
# On replica:
# --source include/sync_replica_io.inc
#
# Parameters:
#   $use_gtids
#     If set, uses GTIDs instead of filename and offset for positions.
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc
#
#   $ignore_gtids_on_sync
#     Forces the use of primary file and position, even if $use_gtids is set.
#     This might be used if the replica will not have all the GTIDs of the primary
#     but have to read and apply all primary events to the end.


--let $include_filename= sync_replica_io.inc
--source include/begin_include_file.inc

--let $_old_replica_error_param= $replica_error_param
--let $replica_error_param= Last_IO_Errno

#
# Verify if the IO thread is already updated by the primary position
# regardless the use of GTIDs. If the IO thread is already updated,
# we can reduce the replica timeout to a minimum because there in nothing
# else to sync.
# As there is a possibility of missing GTIDs on replica, this reduced
# timeout will make mtr throw an error almost promptly, without having
# to wait 300 seconds to notice a sync problem between primary and replica.
#
--let $_replica_primary_file= query_get_value(SHOW REPLICA STATUS, Primary_Log_File, 1)
--let $_replica_primary_pos= query_get_value(SHOW REPLICA STATUS, Read_Primary_Log_Pos, 1)
--let $_saved_replica_timeout= $replica_timeout
if ( $_replica_primary_file == $_saved_file )
{
  if ( $_replica_primary_pos == $_saved_pos )
  {
    if ($rpl_debug)
    {
      --echo debug: It seems that IO thread is already synced (by verifying primary file and position)
      --echo debug: Changing replica timeout to 1 second
    }
    --let $replica_timeout= 1
  }
}

--let $_sync_using_gtids= $use_gtids
if ($ignore_gtids_on_sync)
{
  --let $_sync_using_gtids= 0
}

if ($_sync_using_gtids)
{
  --source include/assert_gtid_mode_on.inc
  --let $replica_param= Retrieved_Gtid_Set
  --let $replica_param_value= $_saved_gtids
  --source include/wait_for_replica_param.inc
}
if (!$_sync_using_gtids)
{
  --let $replica_param= Primary_Log_File
  --let $replica_param_value= $_saved_file
  --source include/wait_for_replica_param.inc

  --let $replica_param= Read_Primary_Log_Pos
  --let $replica_param_value= $_saved_pos
  --source include/wait_for_replica_param.inc
}

--let $replica_error_param= $_old_replica_error_param

--let $replica_timeout= $_saved_replica_timeout

--let $include_filename= sync_replica_io.inc
--source include/end_include_file.inc
