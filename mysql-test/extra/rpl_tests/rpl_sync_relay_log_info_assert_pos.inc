#
# This include is a sub routine of rpl_sync_relay_log_info.inc
#

# Save current primary position
--source include/rpl_connection_primary.inc
--let $primary_file= query_get_value(SHOW PRIMARY STATUS, File, 1)
--let $primary_pos= query_get_value(SHOW PRIMARY STATUS, Position, 1)

# Inc the sub test counter
--inc $_rrli_counter

# Add sub test counter to the header
--let $_rrli_subheader= $_rrli_header, step=$_rrli_counter

# Get the expected status of the SQL thread positions
--let $_rrli_expected= `SELECT SUBSTR("$rpl_relay_log_info_expected_update", $_rrli_counter, 1)`

if ($_rrli_expected != "N")
{
  if ($_rrli_expected != "U")
  {
    --echo ERROR IN TEST: rpl_relay_log_info_expected_update parameter: U or N is expected at position $_rrli_counter
    --die Please fix your test case!
  }
}
--source include/rpl_connection_replica.inc

# When the primary flushed the binary log, this will trigger the relay log to
# be rotated and the relay log info will be flushed once the SQL thread
# reaches the rotate.
# The info obtained in the beginning of this include will show that the primary
# position is right after binary log FD + Previous_gtids, but the
# primary_position flushed by the replica in the relay info table repository in
# this case will be the beginning of the primary's binary log (4).
# Without the following workaround this include would generate an assertion
# failure as the information stored on the repository was updated to (4) but
# this value is different from primary's actual position (around 194 when this
# test case was designed).
if ($_rrli_counter == 4)
{
  if (`SELECT @@GLOBAL.sync_relay_log_info = 0`)
  {
    --let $primary_pos= 4
  }
}

# Ensure that the SQL thread is waiting for more events if started
--let $is_replica_sql_running= query_get_value(SHOW REPLICA STATUS $_for_channel, Replica_SQL_Running, 1)
if ($is_replica_sql_running == 'Yes' )
{
  --let $replica_param= Replica_SQL_Running_State
  --let $replica_param_value= Replica has read all relay log; waiting for more updates
  --source include/wait_for_replica_param.inc
}

# If it was expected to not be updated
if ( $_rrli_expected == "N" )
{
  --let $assert_text= ($_rrli_subheader) SQL thread did not synced primary binlog file
  --let $assert_cond= [ SELECT Primary_log_name = "$last_synced_primary_file" FROM mysql.replica_relay_log_info WHERE Channel_name = "" ]
  --source include/assert.inc

  --let $assert_text= ($_rrli_subheader) SQL thread did not synced primary binlog position
  --let $assert_cond= [ SELECT Primary_log_pos = "$last_synced_primary_pos" FROM mysql.replica_relay_log_info WHERE Channel_name = "" ]
  --source include/assert.inc
}

# If it was expected to be updated
if ( $_rrli_expected == "U" )
{
  --let $assert_text= ($_rrli_subheader) SQL thread synced primary binlog file
  --let $assert_cond= [ SELECT Primary_log_name = "$primary_file" FROM mysql.replica_relay_log_info WHERE Channel_name = "" ]
  --source include/assert.inc
  --let $last_synced_primary_file= $primary_file

  --let $assert_text= ($_rrli_subheader) SQL thread synced primary binlog position
  --let $assert_cond= [ SELECT Primary_log_pos = "$primary_pos" FROM mysql.replica_relay_log_info WHERE Channel_name = "" ]
  --source include/assert.inc
  --let $last_synced_primary_pos= $primary_pos
}

--source include/gtid_step_assert.inc
