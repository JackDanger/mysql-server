#
# Similar to sync_replica_with_primary but supports a configurable timeout
#  let $replica_sync_timeout =  300;
#  source include/wait_for_replica_to_sync_with_primary;
#

connection replica;
stop replica;
start replica;

connection primary;

let $file = query_get_value(SHOW PRIMARY STATUS, File, 1);
let $pos = query_get_value(SHOW PRIMARY STATUS, Position, 1);

connection replica;
--disable_result_log
--disable_query_log
eval select primary_pos_wait("$file", $pos, $replica_sync_timeout);
--enable_result_log
--enable_query_log

connection primary;
sync_replica_with_primary;

connection primary;

