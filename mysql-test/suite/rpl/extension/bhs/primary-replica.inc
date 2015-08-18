# Replication tests need binlog
#
# $skip_replica_start  If true, the replica will not be started
source include/have_log_bin.inc;

connect (primary,127.0.0.1,root,,test,$PRIMARY_MYPORT,);
connect (primary1,127.0.0.1,root,,test,$PRIMARY_MYPORT,);
connect (replica,127.0.0.1,root,,test,$REPLICA_MYPORT,);
connect (replica1,127.0.0.1,root,,test,$REPLICA_MYPORT,);

-- source include/primary-replica-reset.inc

source suite/rpl_bhs/extension/bhs/primary-replica-bhs.inc;

if (!$skip_replica_start) {
  connection primary;
  sync_replica_with_primary blackhole_replica;
  sync_replica_with_primary;
}


# Set the default connection to 'primary'
connection primary;
