--source include/not_embedded.inc
--source suite/federated/include/have_federated_db.inc

connect (primary,127.0.0.1,root,,test,$PRIMARY_MYPORT,);
connect (replica,127.0.0.1,root,,test,$REPLICA_MYPORT,);

connection primary;
CREATE DATABASE federated;

connection replica;
CREATE DATABASE federated;
