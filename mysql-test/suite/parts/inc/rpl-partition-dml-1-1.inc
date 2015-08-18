let $DATA_PATH=;
let $INDEX_PATH=;
--source include/have_partition.inc
--source include/primary-replica.inc
set @old_global_binlog_format=  @@global.binlog_format;
set @old_session_binlog_format=  @@session.binlog_format;
SET GLOBAL binlog_format = 'ROW';
SET SESSION binlog_format = 'ROW';
select @@global.binlog_format, @@session.binlog_format;
--echo [on replica]
connection replica;
set @old_global_binlog_format=  @@global.binlog_format;
set @old_session_binlog_format=  @@session.binlog_format;
SET GLOBAL binlog_format = 'ROW';
SET SESSION binlog_format = 'ROW';
select @@global.binlog_format, @@session.binlog_format;
# restart replica so that replica sql thread's binlog format is re-read
# from @@global.binlog_format
--source include/stop_replica.inc
--source include/start_replica.inc

connection primary;

--echo # Additional tests for WL#5217 by QA, testplan 1.1

let $TABLENAME= t1;
--source suite/parts/inc/partition-dml_cr_tab.inc
--source suite/parts/inc/partition-dml_ins_tab.inc

--enable_connect_log

--sorted_result
SELECT * FROM t1 PARTITION (`p0-29`);
UPDATE t1 PARTITION (`p0-29`) SET b='p0-29-upd-1' WHERE a BETWEEN 0 AND 9;
--sorted_result
SELECT * FROM t1 PARTITION (`p0-29`);

--sync_replica_with_primary
--source include/check_replica_is_running.inc
--sorted_result
SELECT * FROM t1 PARTITION (`p0-29`);

connection primary;
UPDATE t1 PARTITION (`p30-299`) SET b='p30-299-upd' WHERE a BETWEEN 30 AND 39;
--sorted_result
SELECT * FROM t1 PARTITION (`p30-299`);

--sync_replica_with_primary
--source include/check_replica_is_running.inc
--sorted_result
SELECT * FROM t1 PARTITION (`p30-299`);

connection primary;
UPDATE t1 PARTITION (`p300-2999`) SET b='p300-2999-upd' WHERE a BETWEEN 300 AND 309;
--sorted_result
SELECT * FROM t1 PARTITION (`p300-2999`);

--sync_replica_with_primary
--source include/check_replica_is_running.inc
--sorted_result
SELECT * FROM t1 PARTITION (`p300-2999`);

connection primary;
UPDATE t1 PARTITION (subp3) SET b='subp3-upd';
UPDATE t1 PARTITION (subp4) SET b='subp4-upd';
UPDATE t1 PARTITION (subp5) SET b='subp5-upd';
--sorted_result
SELECT * FROM t1 PARTITION (subp4,subp5,subp3) order by b,a;

--sync_replica_with_primary
--source include/check_replica_is_running.inc
--sorted_result
SELECT * FROM t1 PARTITION (subp4,subp5,subp3) order by b,a;
set @@global.binlog_format=  @old_global_binlog_format;
set @@session.binlog_format=  @old_session_binlog_format;

connection primary;
set @@global.binlog_format=  @old_global_binlog_format;
set @@session.binlog_format=  @old_session_binlog_format;
DROP TABLE t1;

--disable_connect_log
