#
# now setup replication to continue from last epoch
# 1. get ndb_apply_status epoch from replica
# 2. get corresponding _next_ binlog postition from primary
# 3. change primary on replica

# 1.

--connection replica
--replace_column 1 <the_epoch>
SELECT @the_epoch:=MAX(epoch) FROM mysql.ndb_apply_status;
--let $the_epoch= `select @the_epoch`

# 2.
--connection primary
--replace_result $the_epoch <the_epoch>
--replace_column 1 <the_pos>
--disable_query_log
eval SELECT @the_pos:=Position,
   @the_file:=SUBSTRING_INDEX(REPLACE(FILE,'\\\\','/'), '/', -1)
   FROM mysql.ndb_binlog_index WHERE epoch > $the_epoch ORDER BY epoch ASC LIMIT 1;
--enable_query_log
--let $the_pos= `SELECT @the_pos`
--let $the_file= `SELECT @the_file`

# 3.
--connection replica
--replace_result $the_pos <the_pos>
eval CHANGE PRIMARY TO
  primary_log_file = '$the_file',
  primary_log_pos = $the_pos ;
