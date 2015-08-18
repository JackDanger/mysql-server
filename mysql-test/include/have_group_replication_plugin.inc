--let $include_filename= have_group_replication_plugin.inc
--let $_previous_include_silent= $include_silent
--let $include_silent= 1
--source include/begin_include_file.inc

# Synchronous replication will work only for RBR mode of replication thus tests
# using it are restricted to this mode only.
--source include/have_log_bin.inc
--source include/have_binlog_format_row.inc
--source include/not_embedded.inc
--source include/have_innodb.inc
--source include/have_gtid.inc

#
# Check if server has support for loading plugins
#
if (`SELECT @@have_dynamic_loading != 'YES'`) {
  --skip Requires dynamic loading
}

if (!`select count(*) from information_schema.plugins where PLUGIN_NAME like 'group_replication'`)
{
  --skip Need Group Replication plugin
}

# Allow $SERVER_MYPORT_1 as alias for $PRIMARY_MYPORT
# necessary for the rpl_group_replication_default_connections.inc file.
if (!$SERVER_MYPORT_1)
{
  --let SERVER_MYPORT_1= $PRIMARY_MYPORT
}

# Allow $SERVER_MYPORT_2 as alias for $REPLICA_MYPORT
# necessary for the rpl_group_replication_default_connections.inc file.
if (!$SERVER_MYPORT_2)
{
  --let SERVER_MYPORT_2= $REPLICA_MYPORT
}

--source include/rpl_group_replication_default_connections.inc

--let $include_filename= have_group_replication_plugin.inc
--let $include_silent= $_previous_include_silent
--source include/end_include_file.inc
