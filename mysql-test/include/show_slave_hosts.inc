--replace_result $REPLICA_MYPORT REPLICA_PORT $DEFAULT_PRIMARY_PORT DEFAULT_PORT
--replace_regex /[0-9, a-f]{8}-[0-9, a-f]{4}-[0-9, a-f]{4}-[0-9, a-f]{4}-[0-9, a-f]{12}/REPLICA_UUID/
SHOW REPLICA HOSTS;
