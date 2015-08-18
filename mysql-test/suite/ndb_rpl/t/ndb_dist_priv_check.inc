if (!$cmd)
{
  die Missing argument $cmd;
}

if (!$chk)
{
  die Missing argument $chk;
}

# Run cmd on current connection, allow cmd to fail
# if $cmd_error is set
let $_err= 0;
if ($cmd_error)
{
  let $_err = $cmd_error;
}
--error $_err
eval $cmd;

# Check result on primary servers
--connection server1
let $result_server1= `$chk`;
--connection server2
let $result_server2= `$chk`;

if ($result_server1 != $result_server2)
{
  echo cmd: $cmd;
  echo result_server1: $result_server1;
  echo result_server2: $result_server2;
  die result differed between server1 and server2;
}

# Sync replica with primary and check against primary
--connection server1
let $save_include_silent= $include_silent;
let $include_silent= 1;
--source include/sync_replica_sql_with_primary.inc
let $include_silent= $save_include_silent;

--connection replica
let $result_replica= `$chk`;

if ($result_server1 != $result_replica)
{
  echo cmd: $cmd;
  echo result_server1: $result_server1;
  echo result_replica: $result_replica;
  die result differed between server1 and replica;
}
echo OK;
echo;

# Reset the input variables to detect missing param setup
let $cmd=;
let $chk=;
let $cmd_error=;
