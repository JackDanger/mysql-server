--connection replica
--source suite/ndb_rpl/t/ndb_conflict_info_init.inc

--connection primary

--eval create table t1 (a int, b varchar(32), X $col_type, d varchar(9), primary key (a, d)) engine ndb
create table t2 (a int key, b varchar(32), c int unsigned, d varchar(9)) engine ndb;

--echo ************************************************
--echo * some basic positive testing, i.e. no conflicts
--echo ************************************************
--connection primary
--echo *** insert some data
begin;
insert into t1 values (1, "Primary t1 a=1 at X=1", 1, '1');
insert into t1 values (2, "Primary t1 a=2 at X=1", 1, '12');
insert into t1 values (3, "Primary t1 a=3 at X=1", 1, '123');
insert into t2 values (1, "Primary t2 a=1 at c=1", 1, '1');
insert into t2 values (2, "Primary t2 a=2 at c=1", 1, '12');
insert into t2 values (3, "Primary t2 a=3 at c=1", 1, '123');
commit;
--sync_replica_with_primary

--echo *** primary - update all
--connection primary
begin;
update t1 set b="Primary t1 X=2", X=X+1;
update t2 set b="Primary t2 c=2", c=c+1;
commit;
--echo *** replica - check update all
--sync_replica_with_primary
--connection replica
select * from t1 order by a, d;
select * from t2 order by a, d;
--source suite/ndb_rpl/t/ndb_conflict_info.inc

--echo *** primary - update 2 rows in different tables
--connection primary
update t1 set b="Primary t1 a=2 at X=3", X=X+1 where a=2;
update t2 set b="Primary t2 a=3 at c=3", c=c+1 where a=3;
--echo *** replica - check update 2 rows in different tables
--sync_replica_with_primary
--connection replica
select * from t1 order by a, d;
select * from t2 order by a, d;
--source suite/ndb_rpl/t/ndb_conflict_info.inc

--echo *** primary - delete all
--connection primary
delete from t1;
delete from t2;
--echo *** replica - check delete all
--sync_replica_with_primary
--connection replica
select * from t1 order by a, d;
select * from t2 order by a, d;
--source suite/ndb_rpl/t/ndb_conflict_info.inc
--source suite/ndb_rpl/t/ndb_conflict_info_init.inc

--echo ************************
--echo * start conflict testing
--echo ************************
--echo *** primary - insert some data
--connection primary
begin;
insert into t1 values (1, "Primary t1 a=1 at X=10", 10, '111');
insert into t1 values (2, "Primary t1 a=2 at X=10", 10, '111222');
insert into t1 values (3, "Primary t1 a=3 at X=10", 10, '111222333');
insert into t1 values (4, "Primary t1 a=4 at X=10", 10, '111222333');
insert into t2 values (1, "Primary t2 a=1 at c=10", 10, '111');
insert into t2 values (2, "Primary t2 a=2 at c=10", 10, '111222');
insert into t2 values (3, "Primary t2 a=3 at c=10", 10, '111222333');
insert into t2 values (4, "Primary t2 a=4 at c=10", 10, '111222333');
commit;

--echo *** replica - check conflict info, no conflicts yet
--sync_replica_with_primary
--connection replica
--source suite/ndb_rpl/t/ndb_conflict_info.inc

--echo *** replica - check insert some data
select * from t1 order by a, d;
select * from t2 order by a, d;

--echo *** replica - update some data to cause conflicts
begin;
update t1 set b="Replica t1 a=1 at X=12", X=12 where a=1 and d='111';
update t1 set b="Replica t1 a=2 at X=12", X=12 where a=2 and d='111222';
update t1 set b="Replica t1 a=3 at X=12", X=12 where a=3 and d='111222333';
update t1 set b="Replica t1 a=4 at X=12", X=12 where a=4 and d='111222333';
update t2 set b="Replica t2 a=1 at c=12", c=12 where a=1;
update t2 set b="Replica t2 a=2 at c=12", c=12 where a=2;
update t2 set b="Replica t2 a=3 at c=12", c=12 where a=3;
update t2 set b="Replica t2 a=4 at c=12", c=12 where a=4;
commit;

--echo *** primary - update some data to cause conflicts
--connection primary
begin;
update t1 set b="Primary t1 a=1 at X=11", X=11 where a=1 and d='111';
update t1 set b="Primary t1 a=2 at X=11", X=11 where a=2 and d='111222';
update t1 set b="Primary t1 a=3 at X=11", X=11 where a=3 and d='111222333';
delete from t1 where a = 4;
update t2 set b="Primary t2 a=1 at c=11", c=11 where a=1;
update t2 set b="Primary t2 a=2 at c=11", c=11 where a=2;
update t2 set b="Primary t2 a=3 at c=11", c=11 where a=3;
delete from t2 where a = 4;
commit;

--echo *** replica - check conflict info, there should be some
--sync_replica_with_primary
--connection replica
--source suite/ndb_rpl/t/ndb_conflict_info.inc

--echo *** replica - check update some data that causes conflicts
select * from t1 order by a, d;
select * from t2 order by a, d;
--echo *** replica - correct some data to correspond to the primary
update t1 set b="Replica t1 a=1 at X=11", X=11 where a=1 and d='111';
delete from t1 where a = 4;

--echo *** primary - update some data, use higher timestamp
--connection primary
begin;
update t1 set b="Primary t1 a=1 at X=13", X=13 where a=1 and d='111';
update t1 set b="Primary t1 a=2 at X=13", X=13 where a=2 and d='111222';
update t1 set b="Primary t1 a=3 at X=13", X=13 where a=3 and d='111222333';
update t2 set b="Primary t2 a=1 at c=13", c=13 where a=1;
update t2 set b="Primary t2 a=2 at c=13", c=13 where a=2;
update t2 set b="Primary t2 a=3 at c=13", c=13 where a=3;
commit;

--echo *** replica - check conflict info, change depends on calling test
--sync_replica_with_primary
--connection replica
--source suite/ndb_rpl/t/ndb_conflict_info.inc

--echo *** replica - check higer timestamp
select * from t1 order by a, d;
select * from t2 order by a, d;

--connection primary
drop table t1, t2;
