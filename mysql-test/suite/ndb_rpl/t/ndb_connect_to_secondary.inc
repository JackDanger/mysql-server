# Use a variable to decide which cluster is 'Secondary'
if (!$SWITCH_CLUSTER_ROLES)
{
  --connection replica
}
if ($SWITCH_CLUSTER_ROLES)
{
  --connection primary
}
