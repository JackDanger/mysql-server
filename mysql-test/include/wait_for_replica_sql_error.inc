# ==== Purpose ====
#
# Waits until the SQL thread of the current connection has got an
# error, or until a timeout is reached. Also waits until the SQL
# thread has completely stopped.
#
# ==== Usage ====
#
# --let $replica_sql_errno= NUMBER [, NUMBER ...] [# comment]
# [--let $show_replica_sql_error= 1]
# [--let $rpl_channel_name= NAME]
# [--let $rpl_debug= 1]
# [--let $replica_timeout= NUMBER]
# --source include/wait_for_replica_sql_error.inc
#
# Parameters:
#   $replica_sql_errno
#     The expected SQL error numbers. This can be either a single
#     number, or a comma-separated list of numbers. Examples:
#       --let $replica_sql_errno= 1040, 1053, 2002, 2003, 2006, 2013
#       --let $replica_sql_errno= 1593 # ER_REPLICA_FATAL_ERROR
#     It is allowed to add a comment at the end of the line, and it
#     is strongly advised to put the symbolic name of the error in
#     this comment, like the example above.
#     (After BUG#41956 has been fixed, this will be required to be a
#     symbolic name instead of a number.)
#
#   $show_replica_sql_error
#     If set, will print the error to the query log.
# 
#   $rpl_channel_name
#     See include/wait_for_replica_param.inc
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
# 
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= wait_for_replica_sql_error.inc [errno=$replica_sql_errno]
--source include/begin_include_file.inc


# Note: here, we should not wait for Replica_SQL_Errno!=0.
# Replica_SQL_Errno and Replica_SQL_Running are not set atomically.
# Replica_SQL_Errno is set first, then Replica_SQL_Running.  So we wait
# until Replica_SQL_Running=No to be sure that both conditions hold.
let $replica_param= Replica_SQL_Running;
let $replica_param_value= No;
source include/wait_for_replica_param.inc;

let $_wait_for_replica_sql_errno= query_get_value($replica_param_statement, Last_SQL_Errno, 1);

# Check sanity
if ($replica_sql_errno == '') {
  --echo !!!ERROR IN TEST: you must set \$replica_sql_errno before you source
  --echo !!!wait_for_replica_sql_error.inc. The error we got this time was '$_wait_for_replica_sql_errno',
  --echo !!!so you probably want to add the following line to your test case:
  --echo !!!  --let \$replica_sql_errno= $_wait_for_replica_sql_errno
  --die !!!ERROR IN TEST: you must set \$replica_sql_errno before sourcing wait_for_replica_sql_error.inc
}

# Strip away comment
--let $_replica_sql_errno= `SELECT IF(LOCATE('#', '$replica_sql_errno') != 0, SUBSTR('$replica_sql_errno', 1, LOCATE('#', '$replica_sql_errno') - 1), '$replica_sql_errno')`

if ($rpl_debug)
{
  --echo # debug: replica_sql_errno='$replica_sql_errno'; stripped from comments='$_replica_sql_errno' wait_for_replica_sql_errno='$_wait_for_replica_sql_errno'
}

if (`SELECT $_wait_for_replica_sql_errno NOT IN ($_replica_sql_errno)`) {
  --echo **** Replica stopped with wrong error code: $_wait_for_replica_sql_errno (expected $replica_sql_errno) ****
  source include/show_rpl_debug_info.inc;
  --echo **** Replica stopped with wrong error code: $_wait_for_replica_sql_errno (expected $replica_sql_errno) ****
  --die Replica stopped with wrong error code
}

if ($show_replica_sql_error)
{
  --let $_wait_for_replica_sql_error_error= query_get_value($replica_param_statement, Last_SQL_Error, 1)
  # remove timestamp and mask absolute path
  --let $_wait_for_replica_sql_error_error= `SELECT REPLACE("$_wait_for_replica_sql_error_error", '$MYSQL_TEST_DIR', 'MYSQL_TEST_DIR')`
  --echo Last_SQL_Error = '$_wait_for_replica_sql_error_error'
}


--let $include_filename= wait_for_replica_sql_error.inc [errno=$replica_sql_errno]
--source include/end_include_file.inc
