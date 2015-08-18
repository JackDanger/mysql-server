# ==== Purpose ====
#
# Clean up replication configuration after using a 2ch
# setup.
# We need to explicitly reset the IGNORE_SERVER_IDS parameters
# on all Servers to avoid testcase check errors.
#
# ==== Usage ====
#
#   [--let $rpl_debug= 1]
#   --source suite/ndb_rpl/ndb_primary-replica_2ch_end.inc
#
# Parameters:
#   $rpl_debug
#     See include/primary-replica.inc

--source include/rpl_stop_replicas.inc
--connection primary
CHANGE PRIMARY TO IGNORE_SERVER_IDS= ();
--connection primary1
CHANGE PRIMARY TO IGNORE_SERVER_IDS= ();
--connection replica
CHANGE PRIMARY TO IGNORE_SERVER_IDS= ();
--connection replica1
CHANGE PRIMARY TO IGNORE_SERVER_IDS= ();
--source include/rpl_start_replicas.inc

--source include/rpl_end.inc
