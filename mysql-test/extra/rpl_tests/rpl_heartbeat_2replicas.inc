#############################################################
# Author: Serge Kozlov <Serge.Kozlov@Sun.COM>
# Date:   02/19/2009
# Purpose: Testing heartbeat for schema
# 1 primary and 2 replicas
#############################################################
--let $rpl_topology= 1->2,1->3
--source include/rpl_init.inc

--let $rpl_connection_name= primary
--let $rpl_server_number= 1
--source include/rpl_connect.inc

--let $rpl_connection_name= replica_1
--let $rpl_server_number= 2
--source include/rpl_connect.inc

--let $rpl_connection_name= replica_2
--let $rpl_server_number= 3
--source include/rpl_connect.inc

#
# Set different heartbeat periods for replicas
#
--connection replica_1
--source include/stop_replica.inc
CHANGE PRIMARY TO PRIMARY_HEARTBEAT_PERIOD = 0.1;
--source include/start_replica.inc
--connection replica_2
--source include/stop_replica.inc
--replace_column 2 ####
CHANGE PRIMARY TO PRIMARY_HEARTBEAT_PERIOD = 1;
--source include/start_replica.inc

#
# Testing heartbeat for one primary and two replicas
#

# Check that heartbeat events sent to both replicas with correct periods
--connection replica_1
let $status_var= replica_received_heartbeats;
let $status_var_value= query_get_value(SHOW STATUS LIKE 'replica_received_heartbeats', Value, 1);
let $status_var_comparsion= >;
--source include/wait_for_status_var.inc
--echo Replica has received heartbeat event
--connection replica_2
let $status_var= replica_received_heartbeats;
let $status_var_value= query_get_value(SHOW STATUS LIKE 'replica_received_heartbeats', Value, 1);
let $status_var_comparsion= >;
--source include/wait_for_status_var.inc
--let $assert_cond= [replica_1:SHOW STATUS LIKE "replica_received_heartbeats", Value, 1] > [replica_2:SHOW STATUS LIKE "replica_received_heartbeats", Value, 1]
--let $assert_text= replica_1 should have received more heartbeats than replica_2
--source include/assert.inc
--echo

# Create topology primary->replica_2->replica_1 and check that replica_1
# receives heartbeat while replica_2 gets data.

# replica_2 was started w/o --log-replica-updates because replica_2 should
# not send data from primary to replica_1

--source include/rpl_stop_replicas.inc
--let $rpl_topology= 1->3->2
--source include/rpl_change_topology.inc
--source include/rpl_start_replicas.inc
--connection replica_1
--source include/stop_replica.inc
--replace_column 2 ####
CHANGE PRIMARY TO PRIMARY_HEARTBEAT_PERIOD=0.1;
--source include/start_replica.inc

# Check heartbeat for new replication channel replica_2->replica
let $status_var= replica_received_heartbeats;
let $status_var_value= query_get_value(SHOW STATUS LIKE 'replica_received_heartbeats', Value, 1);
let $status_var_comparsion= >;
--source include/wait_for_status_var.inc
--echo replica_1 has received heartbeat event
--connection primary
--echo [on primary]
CREATE TABLE t1 (a INT PRIMARY KEY, b VARCHAR(10), c LONGTEXT);
INSERT INTO t1 VALUES (1, 'on primary', '');
SHOW TABLES;
--echo [on replica_2]
--let $sync_replica_connection= replica_2
--source include/sync_replica_sql_with_primary.inc
SHOW TABLES;
let $replica_2_pos_before= query_get_value(SHOW REPLICA STATUS, Read_Primary_Log_Pos, 1);
--echo [on replica_1]
--let $sync_replica_connection= replica_1
--source include/sync_replica_sql_with_primary.inc
SHOW TABLES;
--connection primary
--echo [on primary]
--echo creating updates on primary and send to replica_2 during 5 second
# Generate events on primary and send to replica_2 during 5 second
let $i= 1;
let $j= 1;
let $k= 1;
--disable_query_log
while ($i) {
  eval SET @c_text=REPEAT('1234567890', $j);
  eval UPDATE t1 SET a=$j, c=@c_text;
  --connection replica_2
  let $replica_2_pos= query_get_value(SHOW REPLICA STATUS, Read_Primary_Log_Pos, 1);  
  if (`SELECT ($k*($replica_2_pos - $replica_2_pos_before)) > 0`) {
    --connection replica_1
    let $replica_1_rcvd_heartbeats_before= query_get_value(SHOW STATUS LIKE 'replica_received_heartbeats', Value, 1);    
    let $k= 0;
    let $time_before = `SELECT NOW()`;
  }
  if (`SELECT ((1-$k)*TIMESTAMPDIFF(SECOND,'$time_before',NOW())) > 5`) {
    let $i= 0;
  }
  --connection primary
  inc $j;
  sleep 0.1;
}
--enable_query_log
--connection replica_1
--echo [on replica_1]
--let $assert_cond= [SHOW STATUS LIKE "replica_received_heartbeats", Value, 1] > $replica_1_rcvd_heartbeats_before
--let $assert_text= replica_1 should have received heartbeats
--source include/assert.inc
--echo

#
# Clean up
#
--echo *** Clean up ***
--connection primary
DROP TABLE t1;

--source include/rpl_end.inc
