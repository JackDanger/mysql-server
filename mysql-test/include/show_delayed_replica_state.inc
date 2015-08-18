# ==== Purpose ====
#
# Display the delay state of the SQL thread.
#
# ==== Usage ====
#
# --let $verbose_delayed_replica_state= [0|1]
# --source extra/rpl_tests/show_delayed_replica_state.inc
#
# By default, the output is normalized so that it does not depend on
# exact timing or exact binlog positions. If
# $verbose_delayed_replica_state is set, then it outputs exact times and
# binlog positions. This can be useful for debugging.

--let $_delayed_replica_status= query_get_value(SHOW REPLICA STATUS, Replica_SQL_Running_State, 1)

--let $_delayed_replica_remaining_delay= query_get_value(SHOW REPLICA STATUS, SQL_Remaining_Delay, 1)
--let $_delayed_replica_qualitative_delay= `SELECT CASE WHEN "$_delayed_replica_remaining_delay" = "NULL" THEN "NULL" WHEN "$_delayed_replica_remaining_delay" = "0" THEN "0" ELSE "greater than zero" END`

--let $_delayed_replica_io_pos= query_get_value(SHOW REPLICA STATUS, Read_Primary_Log_Pos, 1)
--let $_delayed_replica_sql_pos= query_get_value(SHOW REPLICA STATUS, Exec_Primary_Log_Pos, 1)
--let $_delayed_replica_qualitative_log_pos= `SELECT IF($_delayed_replica_io_pos > $_delayed_replica_sql_pos, "behind", "in sync with")`

--echo Replica_SQL_Running_State='$_delayed_replica_status'; SQL_Remaining_Delay is $_delayed_replica_qualitative_delay; SQL thread is $_delayed_replica_qualitative_log_pos IO thread

if ($verbose_delayed_replica_state) {
  --echo SQL_Remaining_Delay='$_delayed_replica_remaining_delay'; Read_primary_log_pos='$_delayed_replica_io_pos'; Exec_Primary_Log_Pos='$_delayed_replica_sql_pos'
}
