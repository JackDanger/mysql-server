--echo connect (primary,127.0.0.1,root,,test,PRIMARY_MYPORT);
connect (primary,127.0.0.1,root,,test,$PRIMARY_MYPORT);
--echo connect (replica,$IPv6,root,,test,REPLICA_MYPORT);
connect (replica,$IPv6,root,,test,$REPLICA_MYPORT);
--echo connection primary;
connection primary;
reset primary;
source include/show_primary_status.inc;
save_primary_pos;
--echo connection replica;
connection replica;
reset replica;
let $primary_host= query_get_value(SHOW REPLICA STATUS, Primary_Host, 1);
--echo Primary-Host: $primary_host
eval change primary to primary_host='$IPv6';
let $primary_host= query_get_value(SHOW REPLICA STATUS, Primary_Host, 1);
--echo Primary-Host: $primary_host
--echo disconnect replica;
disconnect replica;
--echo disconnect primary;
disconnect primary;
--echo connection default;
connection default;

