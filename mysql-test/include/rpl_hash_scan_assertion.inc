-- sync_replica_with_primary
-- let $scan_alg= query_get_value(SHOW STATUS LIKE "Replica_rows_last_search_algorithm_used", Value, 1)
if (`SELECT '$scan_alg' <> $expected_alg`)
{
    -- source include/show_rpl_debug_info.inc
    -- echo Unexcepted search algorithm at the replica: got $scan_alg, expected: $expected_alg
    -- die 
}
-- connection primary
