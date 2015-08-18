--let $include_filename= uninstall_semisync.inc
--source include/begin_include_file.inc

--disable_query_log
--connection replica
--source include/stop_replica_io.inc

# Uninstall rpl_semi_sync_replica first
--disable_warnings
UNINSTALL PLUGIN rpl_semi_sync_replica;

--connection primary
# After BUG#17638477 fix, uninstallation of rpl_semi_sync_primary
#  is not allowed when there are semi sync replicas. Hence kill
#  all dump threads before uninstalling it.
SET GLOBAL rpl_semi_sync_primary_enabled = OFF;
--source include/stop_dump_threads.inc
UNINSTALL PLUGIN rpl_semi_sync_primary;
--enable_warnings

--connection replica
--source include/start_replica_io.inc
--enable_query_log

--source include/end_include_file.inc
