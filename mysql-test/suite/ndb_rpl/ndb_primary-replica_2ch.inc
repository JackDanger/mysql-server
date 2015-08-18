# ==== Purpose ====
#
# Set up circular cluster replication where each 
# cluster has two mysqlds and replication directions are 
# following:
#              1       2
#          primary ---> replica  
#           /            \
#     cluster A        cluster B
#           \  3       4 /
#         primary1 <--- replica1
#
# ==== Usage ====
#
#   [--let $rpl_server_count= N]
#   [--let $rpl_skip_check_server_ids= 1]
#   [--let $rpl_skip_reset_primary_and_replica= 1]
#   [--let $rpl_skip_change_primary= 1]
#   [--let $rpl_skip_start_replica= 1]
#   [--let $rpl_debug= 1]
#   [--let $replica_timeout= NUMBER]
#   --source include/ndb_primary-replica_2ch.inc
#
# Parameters:
#   $rpl_server_count, $rpl_skip_check_server_ids,
#   $rpl_skip_reset_primary_and_replica, $rpl_skip_change_primary,
#   $rpl_skip_start_replica, $rpl_debug, $replica_timeout
#     See include/primary-replica.inc

#--let $rpl_debug= 1
--let $rpl_topology= 1->2,4->3
--let $rpl_skip_start_replica= 1
--source include/rpl_init.inc

# Make connections to mysqlds

--let $rpl_connection_name= primary
--let $rpl_server_number= 1
--source include/rpl_connect.inc

--let $rpl_connection_name= primary1
--let $rpl_server_number= 3
--source include/rpl_connect.inc

--let $rpl_connection_name= replica
--let $rpl_server_number= 2
--source include/rpl_connect.inc

--let $rpl_connection_name= replica1
--let $rpl_server_number= 4
--source include/rpl_connect.inc

# Now add IGNORE_SERVER_IDS
--disable_query_log
connection primary;
CHANGE PRIMARY TO IGNORE_SERVER_IDS= (1,3);
connection primary1;
CHANGE PRIMARY TO IGNORE_SERVER_IDS= (1,3);
connection replica;
CHANGE PRIMARY TO IGNORE_SERVER_IDS= (2,4);
connection replica1;
CHANGE PRIMARY TO IGNORE_SERVER_IDS= (2,4);

# Now start replication
--source include/rpl_start_replicas.inc
--enable_query_log

# Check that all mysqld are compiled with ndb support
--let $_rpl_server= 4
while ($_rpl_server)
{
  --connection server_$_rpl_server
  if (`SELECT COUNT(*) = 0 FROM INFORMATION_SCHEMA.ENGINES WHERE engine = 'ndbcluster' AND (support = 'YES' OR support = 'DEFAULT')`)
  {
    --skip Test requires NDB.
  }
  --source include/ndb_not_readonly.inc
  --dec $_rpl_server
}


# Set the default connection to 'primary' (cluster A)
connection primary;
