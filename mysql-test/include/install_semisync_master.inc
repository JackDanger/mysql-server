# Setup semisync primary
# Usage:
# [--let $semisync_primary_enabled = [ON|OFF]]
# --source include/install_semisync_primary.inc
#
# $semisync_primary_enabled is optional. semisync_primary will be enabled if
# $semisync_primary_enabled is not set.

--let $include_filename= install_semisync_primary.inc
--source include/begin_include_file.inc

--source include/not_embedded.inc
--source include/have_semisync_plugin.inc

--disable_query_log

--let $value = query_get_value(show variables like 'rpl_semi_sync_primary_enabled', Value, 1)
if ($value == No such row)
{
  eval INSTALL PLUGIN rpl_semi_sync_primary SONAME '$SEMISYNC_PRIMARY_PLUGIN';
  if ($semisync_primary_enabled == '')
  {
    --let $semisync_primary_enabled= ON
  }
  eval SET GLOBAL rpl_semi_sync_primary_enabled = $semisync_primary_enabled;
}

SET @save_sql_log_bin= @@sql_log_bin;
SET @@sql_log_bin = 0;
CALL mtr.add_suppression(".*Got an error reading communication packets.*");
SET @@sql_log_bin = @save_sql_log_bin;
--source include/end_include_file.inc
