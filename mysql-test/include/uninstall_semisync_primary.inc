# Uninstall semisync primary plugin
# Usage:
# --source include/uninstall_semisync_primary.inc

--let $include_filename= uninstall_semisync_primary.inc
--source include/begin_include_file.inc

--disable_query_log
--disable_warnings
UNINSTALL PLUGIN rpl_semi_sync_primary;

--source include/end_include_file.inc
