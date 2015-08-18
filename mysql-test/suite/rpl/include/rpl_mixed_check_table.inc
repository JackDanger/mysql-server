#########################################
# Author: Serge Kozlov skozlov@mysql.com
# Date: 07/10/2006
# Purpose: Checking created tables on primary and same tables on replica 
# Requirements: 
#########################################

--echo ==========PRIMARY==========
SHOW CREATE TABLE t1;
SHOW CREATE TABLE t2;
sync_replica_with_primary;
--echo ==========REPLICA===========
USE test_rpl;
SHOW CREATE TABLE t1;
SHOW CREATE TABLE t2;
connection primary;
