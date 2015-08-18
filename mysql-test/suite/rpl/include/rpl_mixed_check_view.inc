#########################################
# Author: Serge Kozlov skozlov@mysql.com
# Date: 07/10/2006
# Purpose: Showing view from work tables on primary and on replica
# Requirements: 
#########################################

--echo ==========PRIMARY==========
SHOW CREATE VIEW v1;
SELECT * FROM v1 ORDER BY a;
sync_replica_with_primary;
--echo ==========REPLICA===========
USE test_rpl;
SHOW CREATE VIEW v1;
SELECT * FROM v1 ORDER BY a;
connection primary;

