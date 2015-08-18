#
# Show the Ndb_replica_max_replicated_epoch status variable
#
# Note! Using an intermediary mysqltest variable to avoid
# showing deprecation warnings when using information_schema.global_status
#
let $_smre =
  `select variable_value from information_schema.global_status
     where variable_name = 'Ndb_replica_max_replicated_epoch'`;
echo Ndb_replica_max_replicated_epoch: $_smre;
