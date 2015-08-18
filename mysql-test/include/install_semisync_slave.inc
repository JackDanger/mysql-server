# Setup semisync primary
# Usage:
# --source include/install_semisync_replica.inc

--let $include_filename= install_semisync_replica.inc
--source include/begin_include_file.inc

--source include/not_embedded.inc
--source include/not_gtid_enabled.inc
--source include/have_semisync_plugin.inc

--disable_query_log

--source include/stop_replica_io.inc

--let $value= query_get_value(show variables like 'rpl_semi_sync_replica_enabled', Value, 1)
if ($value == No such row)
{
  eval INSTALL PLUGIN rpl_semi_sync_replica SONAME '$SEMISYNC_REPLICA_PLUGIN';
  SET GLOBAL rpl_semi_sync_replica_enabled = 1;
}

--source include/start_replica_io.inc

--source include/end_include_file.inc
