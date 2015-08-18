#
# This is a wrapper to help minimize writing of assertions
# that resort to rpl_row_img_parts_assertion.inc on a 
# primary-replica scenario.
#
# This script takes a statement to be executed and primary
# and replica expected images description as parameters. Before
# statement execution (on the primary only), both servers are
# synchronized, and positions explicitly saved. These are
# then used as starting position in mysqlbinlog -v (see
# suite/rpl/include/rpl_row_img_parts_assertion.inc).
# 
# Parameters:
#  - $row_img_query
#    The query to be executed on the primary, which will
#    make primary and replica log one or more row events.
#
#  - $row_img_expected_primary
#    String representation of the expected images to search
#    on primary's mysqlbinlog -v output
#
#  - $row_img_expected_replica
#    String representation of the expected images to search
#    on replicas's mysqlbinlog -v output
#
# Sample usage:
#
#  -- let $row_img_query= INSERT INTO t VALUES (1, 2, 3)
#  -- let $row_img_expected_primary= | 1:1 2:2 3:3
#  -- let $row_img_expected_replica = | 1:1 2:2 3:3
#  -- source include/rpl_row_img_parts_primary_replica.inc
#
#  -- let $row_img_query= UPDATE t SET c2= 4 WHERE c1=1
#  -- let $row_img_expected_primary= 1:1 | 2:4
#  -- let $row_img_expected_replica = 1:1 2:2 3:3 | 1:1 2:4 3:3
#  -- source include/rpl_row_img_parts_primary_replica.inc
#
#  -- let $row_img_query= DELETE FROM t WHERE c2=4
#  -- let $row_img_expected_primary= 1:1 |
#  -- let $row_img_expected_replica = 1:1 2:4 3:3 | 
#  -- source include/rpl_row_img_parts_primary_replica.inc
#
# NOTES:
#
#  1. Both primary and replica are synchronized before execution
#     of the query takes place
#  
#  2. original connection - the one in use before including
#     this script - is restored at the end

-- let $old_conn= $CURRENT_CONNECTION

SET sql_mode = 'NO_ENGINE_SUBSTITUTION';

-- connection primary
-- sync_replica_with_primary

-- let $row_img_pos_replica= query_get_value(SHOW PRIMARY STATUS, Position, 1)

-- connection primary
-- let $row_img_pos_primary= query_get_value(SHOW PRIMARY STATUS, Position, 1)

# execute the query
-- connection primary
-- eval $row_img_query

# lets wait until the binary log position changes
-- let $_pos= query_get_value("SHOW PRIMARY STATUS", Position, 1)

# wait for 5 minutes
-- let $iterations=3000
while($_pos == $row_img_pos_primary)
{
  -- sleep 0.1
  -- let $_pos= query_get_value("SHOW PRIMARY STATUS", Position, 1)
  -- dec $iterations
}

if (!$iterations)
{
  -- echo Time out while waiting for the event to be written to the binary log at the primary!
  -- echo Query: $row_img_query
  -- source include/show_rpl_debug_info.inc
  -- exit
}

# now sync the replica
-- sync_replica_with_primary

# primary assertion
-- connection primary
-- let $row_img_expected= $row_img_expected_primary
-- let $row_img_pos= $row_img_pos_primary
-- source include/rpl_row_img_parts_assertion.inc

# replica assertion 
-- connection replica
-- let $row_img_expected= $row_img_expected_replica
-- let $row_img_pos= $row_img_pos_replica
-- source include/rpl_row_img_parts_assertion.inc

-- connection $old_conn

SET sql_mode = default;
