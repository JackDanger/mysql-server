# Use a variable to decide which cluster is 'Primary'
if (!$SWITCH_CLUSTER_ROLES)
{
  --connection primary
}
if ($SWITCH_CLUSTER_ROLES)
{
  --connection replica
}
