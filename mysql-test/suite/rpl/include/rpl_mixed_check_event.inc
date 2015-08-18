#########################################
# Author: Serge Kozlov skozlov@mysql.com
# Date: 07/10/2006
# Purpose: Showing events from work tables on primary and on replica
# Requirements: 
#########################################

--echo ==========PRIMARY==========
--replace_column 8 # 9 #
SHOW EVENTS;
sync_replica_with_primary;
--echo ==========REPLICA===========
USE test_rpl;
--replace_column 8 # 9 #
SHOW EVENTS;
connection primary;

