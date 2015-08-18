###############################################################################
# Description: Performs binloggable and non binloggable DDL and DML at each
#              MySQLD in pair of clusters with (at least) 3 MySQLDs each.
#              Then connects to each MySQLD and examines the contents of
#              its Binlog.
#              Expected contents depends on the configuration of Binlogging
#              in the cluster used.
#              Default from rpl_ndb_multi_binlog_update.test is as shown
#              below. 
# 
# Testing scenario: Cluster 1 replicates to Cluster 2
#
#  Key : BL= Binlogging, LSU = LogReplicaUpdates = On
#
#                       BL                        BL
#  cluster 1 [  srv_primary  srv_primary1  srv_primary2  ]
#                   |
#                   |-----------------------
#                   v          v            v
#  cluster 2 [  srv_replica  srv_replica1  srv_replica2 ]
#                                  BL      BL LSU
#
#  Cluster 1:
#  - Schema change originates on all severs in Cluster 1
#  - MySQLD1 which is binlogging primary. 
#    Identification: connection (srv_primary), config (mysqld.1.1).
#  - MySQLD2 which is not binlogging.
#    Identification: connection (srv_primary1), config (mysqld.1.2).
#  - MySQLD3 which is binlogging but not currently primary.
#    Identification: connection (srv_primary2), config (mysqld.1.3).
#
#  Cluster 2:
#  - MySQLD1: 
#    Can act as replica, not binlogging
#    Identification: connection (srv_replica), config (mysqld.1.replica).
#  - MySQLD2: 
#    Can act as replica, binlogging, log-replica-updates = off
#    Identification: connection (srv_replica1), config (mysqld.2.replica).
#  - MySQLD3 
#    Can act as replica, binlogging, log-replica-updates = on
#    Identification: connection (srv_replica2), config (mysqld.3.replica).
#
# See rpl_ndb_multi_binlog_update.test for default configuration.
# Originally motivated by bug#45756
# See rpl_ndb_replica_lsu.test for full testcase
#
# Preconditions :
#   - Connections srv_primary, srv_primary1, srv_primary2, srv_replica, 
#     srv_replica1, srv_replica2 exist.
#   - $which_replica contains the name of the replica server performing
#     the replication replica role.
#
###############################################################################

###############################################################################
#                            Checking Replication
###############################################################################
--echo *** Generating replica cluster originated binloggable changes ***
connection srv_replica;
CREATE TABLE bug_45756_replica_logged_1 (a int) engine = NDB;
INSERT INTO bug_45756_replica_logged_1 VALUES (1);
DROP TABLE bug_45756_replica_logged_1;

connection srv_replica1;
CREATE TABLE bug_45756_replica_logged_2 (a int) engine = NDB;
INSERT INTO bug_45756_replica_logged_2 VALUES (1);
DROP TABLE bug_45756_replica_logged_2;

connection srv_replica2;
CREATE TABLE bug_45756_replica_logged_3 (a int) engine = NDB;
INSERT INTO bug_45756_replica_logged_3 VALUES (1);
DROP TABLE bug_45756_replica_logged_3;

--echo ***Generating replica cluster non-binloggable changes***
connection srv_replica;
SET SQL_LOG_BIN= 0;
CREATE TABLE bug_45756_replica_not_logged_1 (a int) engine = NDB;
INSERT INTO bug_45756_replica_not_logged_1 VALUES (1);
DROP TABLE bug_45756_replica_not_logged_1;
SET SQL_LOG_BIN= 1;

connection srv_replica1;
SET SQL_LOG_BIN= 0;
CREATE TABLE bug_45756_replica_not_logged_2 (a int) engine = NDB;
INSERT INTO bug_45756_replica_not_logged_2 VALUES (1);
DROP TABLE bug_45756_replica_not_logged_2;
SET SQL_LOG_BIN= 1;

connection srv_replica2;
SET SQL_LOG_BIN= 0;
CREATE TABLE bug_45756_replica_not_logged_3 (a int) engine = NDB;
INSERT INTO bug_45756_replica_not_logged_3 VALUES (1);
DROP TABLE bug_45756_replica_not_logged_3;
SET SQL_LOG_BIN= 1;

--echo *** Generating data to be replicated ***
connection srv_primary;
CREATE TABLE bug45756_primary_logged_1 (a int) engine = NDB;
INSERT INTO bug45756_primary_logged_1 VALUES (1);
DROP TABLE bug45756_primary_logged_1;

connection srv_primary1;
CREATE TABLE bug45756_primary_logged_2 (a int) engine = NDB;
INSERT INTO bug45756_primary_logged_2 VALUES (1);
DROP TABLE bug45756_primary_logged_2;

connection srv_primary2;
CREATE TABLE bug45756_primary_logged_3 (a int) engine = NDB;
INSERT INTO bug45756_primary_logged_3 VALUES (1);
DROP TABLE bug45756_primary_logged_3;

--echo *** Generating changes not to be replicated ***
connection srv_primary;
SET SQL_LOG_BIN= 0;
CREATE TABLE bug45756_primary_not_logged_1 (a int) engine = NDB;
INSERT INTO bug45756_primary_not_logged_1 VALUES (1);
DROP TABLE bug45756_primary_not_logged_1;
SET SQL_LOG_BIN= 1;

connection srv_primary1;
SET SQL_LOG_BIN= 0;
CREATE TABLE bug45756_primary_not_logged_2 (a int) engine = NDB;
INSERT INTO bug45756_primary_not_logged_2 VALUES (1);
DROP TABLE bug45756_primary_not_logged_2;
SET SQL_LOG_BIN= 1;

connection srv_primary2;
SET SQL_LOG_BIN= 0;
CREATE TABLE bug45756_primary_not_logged_3 (a int) engine = NDB;
INSERT INTO bug45756_primary_not_logged_3 VALUES (1);
DROP TABLE bug45756_primary_not_logged_3;
SET SQL_LOG_BIN= 1;

connection srv_primary;
sync_replica_with_primary $which_replica;

--echo *** Checking binlog contents on every server in both clusters ***
connection srv_primary;
--echo 
--echo 
--echo 
--echo connection srv_primary;
show variables like 'server_id';
show variables like 'log_bin';
show variables like 'log_replica_updates';
--source include/show_binlog_events.inc
let $BINLOG_FILENAME=primary-bin;
--source suite/ndb_rpl/t/ndb_rpl_get_binlog_events.inc


connection srv_primary1;
--echo 
--echo 
--echo 
--echo connection srv_primary1;
show variables like 'server_id';
show variables like 'log_bin';
show variables like 'log_replica_updates';
--source include/show_binlog_events.inc

connection srv_primary2;
--echo 
--echo 
--echo 
--echo connection srv_primary2;
show variables like 'server_id';
show variables like 'log_bin';
show variables like 'log_replica_updates';
--source include/show_binlog_events.inc
let $BINLOG_FILENAME=primary-bin;
--source suite/ndb_rpl/t/ndb_rpl_get_binlog_events.inc

connection srv_replica;
--echo 
--echo 
--echo 
--echo connection srv_replica;
show variables like 'server_id';
show variables like 'log_bin';
show variables like 'log_replica_updates';
--source include/show_binlog_events.inc

connection srv_replica1;
--echo 
--echo 
--echo 
--echo connection srv_replica1;
show variables like 'server_id';
show variables like 'log_bin';
show variables like 'log_replica_updates';
--source include/show_binlog_events.inc
let $BINLOG_FILENAME=replica-primary-bin;
--source suite/ndb_rpl/t/ndb_rpl_get_binlog_events.inc

connection srv_replica2;
--echo 
--echo 
--echo 
--echo connection srv_replica2;
show variables like 'server_id';
show variables like 'log_bin';
show variables like 'log_replica_updates';
--source include/show_binlog_events.inc
let $BINLOG_FILENAME=replica-primary-bin;
--source suite/ndb_rpl/t/ndb_rpl_get_binlog_events.inc

connection $which_replica;
