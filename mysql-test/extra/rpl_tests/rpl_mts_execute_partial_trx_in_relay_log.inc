# ==== Implementation ====
#
# 1. Set debug point to rotate relay log after table_map_event.
# 2. Wait for io thread to receive table_map_event and stop it.
# 3. Start the io thread after it is stopped.
# 4. Check that there is no failure on replication.
#

# Initial setup
--source include/rpl_connection_primary.inc
CREATE TABLE t1(i INT) ENGINE=InnoDB;
FLUSH LOGS;
--source include/sync_replica_sql_with_primary.inc

# Set debug point to rotate relay log after table_map_event.
SET @save_debug=@@global.debug;
SET GLOBAL DEBUG='d,stop_io_after_reading_table_map_event';
--source include/rpl_connection_primary.inc
INSERT INTO t1 VALUES (1);

--source include/rpl_connection_replica.inc
# Wait for io thread to receive table_map_event and stop it.
--source include/wait_for_replica_io_to_stop.inc
SET GLOBAL DEBUG= @save_debug;

# Start the io thread after it is stopped.
--source include/start_replica_io.inc

# Check that their is no failure on replication.
--source include/rpl_connection_primary.inc
--source include/sync_replica_sql_with_primary.inc

# Cleanup
--source include/rpl_connection_primary.inc
DROP TABLE t1;
--source include/rpl_end.inc
