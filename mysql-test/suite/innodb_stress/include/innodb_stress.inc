# Populate a table with 1000 records. Allow the replica to sync with the primary.
# Run concurrent threads that run OLTP transactions on primary.
# Kill the primary database server at random points.
# Check the table against the replica.
# Reinvoke the threads.


--connection primary
#--let $innodb_index_cluster_optimization_save_primary = `SELECT @@innodb_prefix_index_cluster_optimization`

--connection replica
#--let $innodb_index_cluster_optimization_save_replica = `SELECT @@innodb_prefix_index_cluster_optimization`

# create the directory for temporary log files.
--exec mkdir -p $MYSQL_TMP_DIR/load_generator

if ($fake_changes)
{
  --exec mkdir -p $MYSQL_TMP_DIR/load_generator_replica
}

--connection primary

# since this test generates lot of errors in log, suppress checking errors
call mtr.add_suppression(".*");

--sync_replica_with_primary

--connection primary
--let $pid_file = `SELECT @@pid_file`
--let $crash_num = 0
--let $primary_host = 127.0.0.1
--let $table = test
--let $user = root
--let $checksum = 0
--let $secondary_index_checks = 0

if ($do_checksum)
{
    # populate the table and store its checksum before any load.
    let $exec =
python $MYSQL_BASEDIR/mysql-test/suite/innodb_stress/t/load_generator.py $pid_file $kill_db_after
$num_records 0 0 $user $primary_host $PRIMARY_MYPORT
$table 0 $max_rows $MYSQL_TMP_DIR/load_generator 0 0 0;
    exec $exec;
    let $checksum=query_get_value(CHECKSUM TABLE t1, Checksum, 1);
    # Primary needs to be restarted to start with an empty buffer pool so
    # that logical read ahead gets used.
    let rpl_server_number = 1;
    source include/rpl_restart_server.inc;
    connection replica;
    # Start replica to avoid I/O thread retry errors
    disable_warnings;
    source include/start_replica.inc;
    enable_warnings;
    let $num_records = 0;
}

while ($num_crashes)
{
  connection primary;
  exec echo "restart" > $MYSQLTEST_VARDIR/tmp/mysqld.1.expect;
  if ($crash_num)
  {
    let $num_records = 0; # do not populate the table except for the first run.
  }

  if ($use_blob)
  {
    let $exec =
python $MYSQL_BASEDIR/mysql-test/suite/innodb_stress/t/load_generator.py $pid_file $kill_db_after
$num_records  $num_workers $num_transactions $user $primary_host $PRIMARY_MYPORT
$table 1 $max_rows $MYSQL_TMP_DIR/load_generator 0 $checksum $secondary_index_checks;
  }
  if (!$use_blob)
  {
    let $exec =
python $MYSQL_BASEDIR/mysql-test/suite/innodb_stress/t/load_generator.py $pid_file $kill_db_after
$num_records  $num_workers $num_transactions $user $primary_host $PRIMARY_MYPORT
$table 0 $max_rows $MYSQL_TMP_DIR/load_generator 0 $checksum $secondary_index_checks;
  }

  exec $exec;

  if ($do_crash)
  {
    --echo Wait for reconnect
    enable_reconnect;
    # Call script that will poll the server waiting for it to be back online again
    source include/wait_until_connected_again.inc;
    connection replica;
    source include/wait_until_connected_again.inc;
    connection primary;
  }

  --echo Checksum primary
  let $primary_checksum = query_get_value(CHECKSUM TABLE t1, Checksum, 1);

  # if sync_replica_with_primary had a configurable timeout this would not be needed
  let $replica_sync_timeout = 7200;
  --source include/wait_for_replica_to_sync_with_primary.inc

  connection replica;
  if ($fake_changes)
  {
    --echo applying fake updates to the replica
    let $replica_pid_file = `SELECT @@pid_file`;
    let $replica_exec =
python $MYSQL_BASEDIR/mysql-test/suite/innodb_stress/t/load_generator.py $replica_pid_file $kill_db_after
0  $num_workers $num_transactions $user $primary_host $REPLICA_MYPORT
$table 0 $max_rows $MYSQL_TMP_DIR/load_generator_replica 1 $checksum $secondary_index_checks;
    exec $replica_exec;
  }
  --echo Checksum replica
  let $replica_checksum=query_get_value(CHECKSUM TABLE t1, Checksum, 1);
  let $not_same = `SELECT $primary_checksum-$replica_checksum`;
  if ($not_same)
  {
    let $msg =
The checksums of table t1 for primary and replica do not match for $crash_num th
crash. This may happen if there is a corrupt recovery log or a bug in crash
recovery. You can take a look at the logs in $MYSQL_TMP_DIR/load_generator to see the
queries issued before the crash.;
    echo $msg;

    connection primary;
    eval select * into outfile '$MYSQLTEST_VARDIR/tmp/primary_all' from t1 order by id;
    eval select id into outfile '$MYSQLTEST_VARDIR/tmp/primary_id' from t1 order by id;
    show primary status;

    connection replica;
    eval select * into outfile '$MYSQLTEST_VARDIR/tmp/replica_all' from t1 order by id;
    eval select id into outfile '$MYSQLTEST_VARDIR/tmp/replica_id' from t1 order by id;
    show replica status;

    die;
  }
  dec $num_crashes;
  inc $crash_num;
}

# final cleanup
--connection primary

let $primary=`select count(*) from t1 use index (primary)`;
let $secondary=`select count(*) from t1 use index (msg_i)`;
if ($primary != $secondary)
{
  --echo Secondary index inconsistent!  $primary != $secondary
  --die
}

DROP TABLE t1;

# if sync_replica_with_primary had a configurable timeout this would not be needed
let $replica_sync_timeout = 7200;
--source include/wait_for_replica_to_sync_with_primary.inc

--connection replica
--source include/stop_replica.inc
# For stress tests sometimes the replication thread can not connect to primary
# temporarily. This is either because the primary crashed and it is recovering
# or the primary is too busy and could not service the replica's requests.
# mtr's internal check requires that there be no errors in replica status.
# restarting replication clears the errors.
--source include/start_replica.inc
--source include/stop_replica.inc

connection primary;

# --exec rm -rf $MYSQL_TMP_DIR/load_generator

--connection replica
# Restore the value of variable on replica
--disable_query_log
#eval SET GLOBAL innodb_prefix_index_cluster_optimization = $innodb_index_cluster_optimization_save_replica;
--enable_query_log

--connection primary
# Restore the value of variable on primary
--disable_query_log
#eval SET GLOBAL innodb_prefix_index_cluster_optimization = $innodb_index_cluster_optimization_save_primary;
--enable_query_log
