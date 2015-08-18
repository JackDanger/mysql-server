#! /bin/bash
#
rm -f $MYSQLTEST_VARDIR/std_data_replica_link
ln -s $MYSQLTEST_VARDIR/std_data $MYSQLTEST_VARDIR/std_data_replica_link
