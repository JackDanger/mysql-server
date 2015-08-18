#
# An auxaliary file for asserting semisync primary on
#
--let $semisync_status= query_get_value(SHOW STATUS LIKE 'rpl_semi_sync_primary_status', Value, 1)
--let $assert_text= Semisync primary is on
--let $assert_cond= "$semisync_status" = "ON"
--source include/assert.inc
