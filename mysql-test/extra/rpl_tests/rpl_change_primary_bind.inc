#
# Test CHANGE PRIMARY PRIMARY_BIND=xxx
#
# Parameters:
#  $primary_bind - the address to use for PRIMARY_BIND
#  $primary_bind_error_expected - expect an error when using the specified
#                                primary_bind address
#
#

# Stop the replica
connection replica;
source include/stop_replica.inc;

# Create table and insert one record with the bind address on primary
connection primary;
create table t1(n int, b varchar(256));
--replace_result $primary_bind <primary_bind>
eval insert into t1 values(1, $primary_bind);

# Configure replica to connect to primary with the give bind address
# for primary_bind and primary_host unless it's an invalid address
connection replica;
let $_primary_host=;
if (!$primary_bind_error_expected)
{
  if ($primary_bind != "''")
  {
    let $_primary_host=primary_host=$primary_bind,;
  }
}

--replace_result $primary_bind <primary_bind>
eval change primary to $_primary_host primary_bind=$primary_bind;
start replica;

# Check that SHOW REPLICA STATUS has Primary_bind column set to $primary_bind
let $primary_bind_value= query_get_value(SHOW REPLICA STATUS, Primary_Bind, 1);
if (`select '$primary_bind_value' != $primary_bind`)
{
  source include/show_rpl_debug_info.inc;
  echo 'primary_bind_value: $primary_bind_value' != 'primary_bind: $primary_bind';
  die Primary_bind in SHOW REPLICA STAUS not showing configured value;
}

if ($primary_bind_error_expected)
{
  # The given primary bind address is not valid
  # and replication should fail
  let $replica_io_errno= $primary_bind_error_expected;
  let $replica_io_error_is_nonfatal= 1;
  source include/wait_for_replica_io_error.inc;
  echo got expected error $primary_bind_error_expected;
  source include/stop_replica.inc;

  # Reset the primary_bind so that cleanup can run
  eval change primary to primary_bind='';
  start replica;

}

source include/wait_for_replica_to_start.inc;

connection primary;
sync_replica_with_primary;

connection replica;
let $primary_bind_repl= query_get_value(select b from t1, b, 1);
if (`select '$primary_bind_repl' != $primary_bind`)
{
  select * from t1;
  source include/show_rpl_debug_info.inc;
  echo 'primary_bind_repl: $primary_bind_repl' != 'primary_bind: $primary_bind';
  die The replicated value to show replication working was not correct;
}

# Clean up
connection primary;
drop table t1;
sync_replica_with_primary;
