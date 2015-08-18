# Insert blackhole replica into regular replication chain
# We hide any output below due using untouched result files of rpl suite

--disable_warnings
--disable_query_log

connection primary;

# Connect blackhole replica to primary.
connect (blackhole_replica,127.0.0.1,root,,test,$BHS_MYPORT,);
connection blackhole_replica;
source include/have_blackhole.inc;
SET default_storage_engine=BLACKHOLE;
STOP REPLICA;
source include/wait_for_replica_to_stop.inc;
RESET REPLICA;
eval CHANGE PRIMARY TO PRIMARY_USER='root', PRIMARY_CONNECT_RETRY=1, PRIMARY_HOST='127.0.0.1', PRIMARY_PORT=$PRIMARY_MYPORT;
START REPLICA;
source include/wait_for_replica_to_start.inc;

# Stop replica and reconnect to blackhole replica
connection replica;
STOP REPLICA;
RESET REPLICA;
eval CHANGE PRIMARY TO PRIMARY_USER='root', PRIMARY_CONNECT_RETRY=1, PRIMARY_HOST='127.0.0.1', PRIMARY_PORT=$BHS_MYPORT;
START REPLICA;
source include/wait_for_replica_to_start.inc;

--enable_query_log
--enable_warnings
