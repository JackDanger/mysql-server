#########################################
# Author: Serge Kozlov skozlov@mysql.com
# Date: 07/10/2006
# Purpose: Showing all databases started from "test_rpl"
# Requirements: 
#########################################

--echo ==========PRIMARY==========
SHOW DATABASES LIKE 'test_rpl%';
sync_replica_with_primary;
--echo ==========REPLICA===========
SHOW DATABASES LIKE 'test_rpl%';
connection primary;
