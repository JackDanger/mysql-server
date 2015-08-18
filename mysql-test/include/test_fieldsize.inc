#
# include/test_fieldsize.inc 
# 
# This include file is designed to create a table with one column
# whose size on the primary is greater than that on the replica. The
# test should fail with an error on the replica.
#

connection primary;
DROP TABLE IF EXISTS t1;

sync_replica_with_primary;
STOP REPLICA;
eval $test_table_replica;

connection primary;
eval $test_table_primary;

--let $rpl_only_running_threads= 1
--source include/rpl_reset.inc

eval $test_insert;

connection replica;
--let $replica_sql_errno= convert_error(ER_REPLICA_CONVERSION_FAILED) # 1677
--source include/wait_for_replica_sql_error.inc

# The following should be 0
SELECT COUNT(*) FROM t1;
STOP REPLICA;
--source include/rpl_reset.inc
--let $rpl_only_running_threads= 0
