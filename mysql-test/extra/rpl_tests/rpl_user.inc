# BUG#33862 completely failed DROP USER statement gets replicated
--source include/primary-replica.inc

#
# remove all users will be used in the test
#
connection primary;
set session sql_log_bin=0;
delete from mysql.user where Host='fakehost';
set session sql_log_bin=1;

connection replica;
set session sql_log_bin=0;
delete from mysql.user where Host='fakehost';
set session sql_log_bin=1;

#
# Test create user
#
connection primary;
create user 'foo'@'fakehost';
--error ER_CANNOT_USER
create user 'foo'@'fakehost', 'bar'@'fakehost';
--error ER_CANNOT_USER
create user 'foo'@'fakehost', 'bar'@'fakehost';

# In log event, Plaintext password 'foo1' is replaced by ciphertext.
create user 'foo1'@'fakehost' IDENTIFIED BY 'foo1', 'foo2'@'fakehost'
  IDENTIFIED BY PASSWORD'*1111111111111111111111111111111111111111',
  'foo3'@'fakehost';
--source include/sync_replica_sql_with_primary.inc
select Host,User from mysql.user where Host='fakehost';
--let $diff_tables= primary:mysql.user, replica:mysql.user
source include/diff_tables.inc;

#
# Test rename user
#
connection primary;
rename user 'foo'@'fakehost' to 'foofoo'@'fakehost';
--error ER_CANNOT_USER
rename user 'not_exist_user1'@'fakehost' to 'foobar'@'fakehost', 'bar'@'fakehost' to 'barbar'@'fakehost';
--error ER_CANNOT_USER
rename user 'not_exist_user1'@'fakehost' to 'foobar'@'fakehost', 'not_exist_user2'@'fakehost' to 'barfoo'@'fakehost';

--source include/sync_replica_sql_with_primary.inc
select Host,User from mysql.user where Host='fakehost';

#
# Test drop user
#
connection primary;
drop user 'foofoo'@'fakehost';
drop user 'foo1'@'fakehost', 'foo2'@'fakehost', 'foo3'@'fakehost';
--error ER_CANNOT_USER
drop user 'not_exist_user1'@'fakehost', 'barbar'@'fakehost';
--error ER_CANNOT_USER
drop user 'not_exist_user1'@'fakehost', 'not_exist_user2'@'fakehost';

--source include/sync_replica_sql_with_primary.inc
select Host,User from mysql.user where Host='fakehost';


--echo #
--echo # WL2392: "Change Password at next login" (initial default for root)
--echo #

connection primary;
CREATE USER must_change2@localhost IDENTIFIED BY 'aha';
ALTER USER must_change2@localhost PASSWORD EXPIRE;
--source include/sync_replica_sql_with_primary.inc
select Host,User,password_expired from mysql.user where user='must_change2';

connect(must_change_con_replica,localhost,must_change2,aha,test,$REPLICA_MYPORT,);
connection must_change_con_replica;
--echo # must throw an error
--error ER_MUST_CHANGE_PASSWORD
SELECT USER();
connection primary;
disconnect must_change_con_replica;

connect(must_change_con_primary,localhost,must_change2,aha);
connection must_change_con_primary;
--echo # setting a password unlocks it
SET PASSWORD = 'aha2';
connection primary;
disconnect must_change_con_primary;

--source include/sync_replica_sql_with_primary.inc
connection primary;

connect(must_change_con_replica,localhost,must_change2,aha2,test,$REPLICA_MYPORT,);
connection must_change_con_replica;
--echo # must not throw an error
SELECT USER();
connection primary;
disconnect must_change_con_replica;

DROP USER must_change2@localhost;

--source include/sync_replica_sql_with_primary.inc
connection primary;

#
# show the binlog events on the primary
#
connection primary;
source include/show_binlog_events.inc;
--source include/rpl_end.inc
