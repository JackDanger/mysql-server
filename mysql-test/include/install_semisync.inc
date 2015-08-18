--let $include_filename= install_semisync.inc
--source include/begin_include_file.inc

--connection primary
--source include/install_semisync_primary.inc

--connection replica
--source include/install_semisync_replica.inc

--source include/end_include_file.inc
