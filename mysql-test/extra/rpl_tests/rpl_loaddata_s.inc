# See if the replica logs (in its own binlog, with --log-replica-updates) a
# replicated LOAD DATA INFILE correctly when it has binlog_*_db rules.
# This is for BUG#1100 (LOAD DATA INFILE was half-logged).

-- source include/have_binlog_format_mixed_or_statement.inc
-- source include/primary-replica.inc

connection replica;
# Not sure why we connect to replica and then try to reset primary, but I will leave it [JBM]
reset primary;

connection primary;
# 'test' is the current database
create table test.t1(a int, b int, unique(b));
load data infile '../../std_data/rpl_loaddata.dat' into table test.t1;

# Test logging on replica;
--source include/sync_replica_sql_with_primary.inc
select count(*) from test.t1; # check that LOAD was replicated
source include/show_binlog_events.inc;

# Cleanup
connection primary;
drop table test.t1;
--source include/sync_replica_sql_with_primary.inc
--source include/rpl_end.inc
