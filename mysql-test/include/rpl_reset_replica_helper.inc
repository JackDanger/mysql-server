# Auxiliary file used by rpl_reset.inc
# This file uses $rpl_channel_name set in rpl_for_each_connection.inc

if (!$rpl_reset_replica_all)
{
  if (!$rpl_multi_source)
  {
    RESET REPLICA;
  }
  if ($rpl_multi_source)
  {
    eval RESET REPLICA FOR CHANNEL '$rpl_channel_name';
  }
}

if ($rpl_reset_replica_all)
{
  if (!$rpl_multi_source)
  {
    RESET REPLICA ALL;
  }
  if ($rpl_multi_source)
  {
    eval RESET REPLICA ALL FOR CHANNEL  '$rpl_channel_name';
  }
}

