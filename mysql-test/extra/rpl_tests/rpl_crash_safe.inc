# ==== Purpose ====
#
# This file is an engine to process commands like
# rpl_mixing_engines.inc.  It has the same set of commands, and in
# addition the extra command 'recovery'.  The 'configure' and 'clean'
# commands are also extended version of the same commands in
# rpl_mixing_engines.inc.
#
# ==== Usage ====
#
# --let $commands= command1 command2 ...
# --let $database_name= name
# --let $failure= d,debug_flag
# [--let $verbose= 1]
# --source include/rpl_mixing_engines.inc
#
#   Parameters:
#
#     $commands, $database_name, $verbose
#       See rpl_mixing_engines.inc.
#
#     $failure
#       This debug symbol will be set. See the implementation for
#       details.
#
# ==== Implementation ====
#
# This file has its own code for 'configure' and 'clean'.  For other
# commands, this file first configures the server according to
# $failure, and then delegates the commands to rpl_mixing_engines.inc.

#
# Creates tables used throughout the test and changes the type of the
# mysql.replica_relay_log_info to Innodb.
#
if ($commands == 'configure')
{
  --sync_replica_with_primary
  --source include/stop_replica.inc
  SHOW CREATE TABLE mysql.replica_relay_log_info;
  SHOW CREATE TABLE mysql.replica_worker_info;
  ALTER TABLE mysql.replica_relay_log_info ENGINE= Innodb;
  ALTER TABLE mysql.replica_worker_info ENGINE= Innodb;
  SHOW CREATE TABLE mysql.replica_relay_log_info;
  SHOW CREATE TABLE mysql.replica_worker_info;
  --source include/start_replica.inc

  connection primary;
  --source extra/rpl_tests/rpl_mixing_engines.inc

  --sync_replica_with_primary
  connection primary;
  --let $commands=
}

#
# Cleans the test case by deleting all tables, triggers, procedures and
# functions that were created.
#
if ($commands == 'clean')
{
  connection primary;
  --source extra/rpl_tests/rpl_mixing_engines.inc
  --let $commands=
}

#
# Executes the set of commands defined in $command by calling
# rpl_mixing_engines.inc
#
if ($commands != '')
{
  # 
  # If an fault injection point was defined, stop the SQL THREAD and
  # prepare the replica to be restarted. Otherwise, do nothing.
  #
  if ($failure != '')
  {
    connection replica;
    STOP REPLICA SQL_THREAD;
    source include/wait_for_replica_sql_to_stop.inc;
    --eval SET GLOBAL debug="$failure";
    --exec echo "wait" > $MYSQLTEST_VARDIR/tmp/mysqld.2.expect
  }

  #
  # Prints the expected outcome after the recovery. Check the
  # WL#4801 for further details.
  #
  connection primary;
  if ($failure != '')
  {
    --let $outcome= O2
    if ($failure != 'd,crash_after_commit_and_update_pos')
    {
      if ($failure != 'd,crash_after_apply')
      {
        if ($failure != 'd,crash_after_commit_before_update_pos')
        {
          --let $outcome= O1
        }
      }
    }
    --echo FAILURE $failure and OUTCOME $outcome
  }

  #
  # Executes the set of commands defined in $command by calling
  # rpl_mixing_engines.inc
  #
  connection primary;
  let $primary_before= query_get_value(SHOW PRIMARY STATUS, Position, 1);
  --source extra/rpl_tests/rpl_mixing_engines.inc
  let $primary_after= query_get_value(SHOW PRIMARY STATUS, Position, 1);

  #
  # Restarts the SQL THREAD and waits for the REPLICA to be crashed and
  # re-started.
  #
  connection primary;
  if ($failure != '')
  {
    connection replica;
    --error 0, 2013
    START REPLICA SQL_THREAD;
    --source include/wait_until_disconnected.inc
    --exec echo "restart" > $MYSQLTEST_VARDIR/tmp/mysqld.2.expect
    --let $rpl_server_number= 2
    --source include/rpl_reconnect.inc
    connection replica;
    --let $replica_before= `SELECT primary_log_pos FROM mysql.replica_relay_log_info`
    START REPLICA;
    source include/wait_for_replica_to_start.inc;
  }

  #
  # Syncs the replica with the primary and checks if the replicas applied all
  # changes from the primary and is not out of sync.
  #
  connection primary;
  sync_replica_with_primary;

  --let $replica_after= `SELECT primary_log_pos FROM mysql.replica_relay_log_info`
  if ($replica_after != $primary_after)
  {
    --echo Replica is out of sync.
    --echo PRIMARY_BEFORE $primary_before PRIMARY_AFTER $primary_after REPLICA_BEFORE $replica_before REPLICA_AFTER $replica_after
    connection replica;
    --source include/show_rpl_debug_info.inc
    --die
  }
}

connection primary;
