if (!$SWITCH_CLUSTER_ROLES)
{
  --connection primary
  --sync_replica_with_primary replica
  --connection replica
}
if ($SWITCH_CLUSTER_ROLES)
{
  --connection replica
  --sync_replica_with_primary primary
  --connection primary
}
