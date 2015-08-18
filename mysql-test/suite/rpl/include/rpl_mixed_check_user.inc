#########################################
# Author: Serge Kozlov skozlov@mysql.com
# Date: 07/10/2006
# Purpose: Showing user info on primary and on replica
# Requirements: 
#########################################

--echo ==========PRIMARY==========
SELECT host, user, authentication_string, select_priv FROM mysql.user WHERE user LIKE 'user_test_rpl%';
sync_replica_with_primary;
--echo ==========REPLICA===========
USE test_rpl;
SELECT host, user, authentication_string, select_priv FROM mysql.user WHERE user LIKE 'user_test_rpl%';
connection primary;

