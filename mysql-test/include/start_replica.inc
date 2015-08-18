# ==== Purpose ====
#
# Issues START REPLICA on the current connection. Then waits until both
# the IO and SQL threads have started, or until a timeout is reached.
#
# Please use this instead of 'START REPLICA', to reduce the risk of test
# case bugs.
#
#
# ==== Usage ====
#
# [--let $rpl_channel_name= NAME]
# [--let $replica_timeout= NUMBER]
# [--let $rpl_debug= 1]
# --source include/start_replica.inc
#
# Parameters:
#   $rpl_channel_name
#     If this is not '', a FOR CHANNEL  '$rpl_channel_name' is added.
#
#   $replica_timeout
#     See include/wait_for_replica_param.inc
#
#   $rpl_debug
#     See include/rpl_init.inc


--let $include_filename= start_replica.inc
--source include/begin_include_file.inc


if (!$rpl_debug)
{
  --disable_query_log
}

if ($rpl_channel_name != '')
{
  eval START REPLICA FOR CHANNEL  '$rpl_channel_name';
}
if ($rpl_channel_name == '')
{
  START REPLICA;
}

source include/wait_for_replica_to_start.inc;


--let $include_filename= start_replica.inc
--source include/end_include_file.inc