/* Copyright (C) 2007 Google Inc.
   Copyright (C) 2008 MySQL AB
   Copyright (c) 2008, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */


#include "semisync_replica.h"
#include <mysql.h>

ReplSemiSyncReplica repl_semisync;

/*
  indicate whether or not the replica should send a reply to the primary.

  This is set to true in repl_semi_replica_read_event if the current
  event read is the last event of a transaction. And the value is
  checked in repl_semi_replica_queue_event.
*/
bool semi_sync_need_reply= false;

C_MODE_START

int repl_semi_reset_replica(Binlog_relay_IO_param *param)
{
  // TODO: reset semi-sync replica status here
  return 0;
}

int repl_semi_replica_request_dump(Binlog_relay_IO_param *param,
				 uint32 flags)
{
  MYSQL *mysql= param->mysql;
  MYSQL_RES *res= 0;
  MYSQL_ROW row;
  const char *query;

  if (!repl_semisync.getReplicaEnabled())
    return 0;

  /* Check if primary server has semi-sync plugin installed */
  query= "SHOW VARIABLES LIKE 'rpl_semi_sync_primary_enabled'";
  if (mysql_real_query(mysql, query, static_cast<ulong>(strlen(query))) ||
      !(res= mysql_store_result(mysql)))
  {
    sql_print_error("Execution failed on primary: %s", query);
    return 1;
  }

  row= mysql_fetch_row(res);
  if (!row)
  {
    /* Primary does not support semi-sync */
    sql_print_warning("Primary server does not support semi-sync, "
                      "fallback to asynchronous replication");
    rpl_semi_sync_replica_status= 0;
    mysql_free_result(res);
    return 0;
  }
  mysql_free_result(res);

  /*
    Tell primary dump thread that we want to do semi-sync
    replication
  */
  query= "SET @rpl_semi_sync_replica= 1";
  if (mysql_real_query(mysql, query, static_cast<ulong>(strlen(query))))
  {
    sql_print_error("Set 'rpl_semi_sync_replica=1' on primary failed");
    return 1;
  }
  mysql_free_result(mysql_store_result(mysql));
  rpl_semi_sync_replica_status= 1;
  return 0;
}

int repl_semi_replica_read_event(Binlog_relay_IO_param *param,
			       const char *packet, unsigned long len,
			       const char **event_buf, unsigned long *event_len)
{
  if (rpl_semi_sync_replica_status)
    return repl_semisync.replicaReadSyncHeader(packet, len,
					     &semi_sync_need_reply,
					     event_buf, event_len);
  *event_buf= packet;
  *event_len= len;
  return 0;
}

int repl_semi_replica_queue_event(Binlog_relay_IO_param *param,
				const char *event_buf,
				unsigned long event_len,
				uint32 flags)
{
  if (rpl_semi_sync_replica_status && semi_sync_need_reply)
  {
    /*
      We deliberately ignore the error in replicaReply, such error
      should not cause the replica IO thread to stop, and the error
      messages are already reported.
    */
    (void) repl_semisync.replicaReply(param->mysql,
                                    param->primary_log_name,
                                    param->primary_log_pos);
  }
  return 0;
}

int repl_semi_replica_io_start(Binlog_relay_IO_param *param)
{
  return repl_semisync.replicaStart(param);
}

int repl_semi_replica_io_end(Binlog_relay_IO_param *param)
{
  return repl_semisync.replicaStop(param);
}

int repl_semi_replica_sql_stop(Binlog_relay_IO_param *param, bool aborted)
{
  return 0;
}

C_MODE_END

static void fix_rpl_semi_sync_replica_enabled(MYSQL_THD thd,
					    SYS_VAR *var,
					    void *ptr,
					    const void *val)
{
  *(char *)ptr= *(char *)val;
  repl_semisync.setReplicaEnabled(rpl_semi_sync_replica_enabled != 0);
  return;
}

static void fix_rpl_semi_sync_trace_level(MYSQL_THD thd,
					  SYS_VAR *var,
					  void *ptr,
					  const void *val)
{
  *(unsigned long *)ptr= *(unsigned long *)val;
  repl_semisync.setTraceLevel(rpl_semi_sync_replica_trace_level);
  return;
}

/* plugin system variables */
static MYSQL_SYSVAR_BOOL(enabled, rpl_semi_sync_replica_enabled,
  PLUGIN_VAR_OPCMDARG,
 "Enable semi-synchronous replication replica (disabled by default). ",
  NULL,				   // check
  &fix_rpl_semi_sync_replica_enabled, // update
  0);

static MYSQL_SYSVAR_ULONG(trace_level, rpl_semi_sync_replica_trace_level,
  PLUGIN_VAR_OPCMDARG,
 "The tracing level for semi-sync replication.",
  NULL,				  // check
  &fix_rpl_semi_sync_trace_level, // update
  32, 0, ~0UL, 1);

static SYS_VAR* semi_sync_replica_system_vars[]= {
  MYSQL_SYSVAR(enabled),
  MYSQL_SYSVAR(trace_level),
  NULL,
};


/* plugin status variables */
static SHOW_VAR semi_sync_replica_status_vars[]= {
  {"Rpl_semi_sync_replica_status",
   (char*) &rpl_semi_sync_replica_status, SHOW_BOOL, SHOW_SCOPE_GLOBAL},
  {NULL, NULL, SHOW_BOOL, SHOW_SCOPE_GLOBAL},
};

Binlog_relay_IO_observer relay_io_observer = {
  sizeof(Binlog_relay_IO_observer), // len

  repl_semi_replica_io_start,	// start
  repl_semi_replica_io_end,	// stop
  repl_semi_replica_sql_stop,     // stop sql thread
  repl_semi_replica_request_dump,	// request_transmit
  repl_semi_replica_read_event,	// after_read_event
  repl_semi_replica_queue_event,	// after_queue_event
  repl_semi_reset_replica,	// reset
};

static int semi_sync_replica_plugin_init(void *p)
{
  if (repl_semisync.initObject())
    return 1;
  if (register_binlog_relay_io_observer(&relay_io_observer, p))
    return 1;
  return 0;
}

static int semi_sync_replica_plugin_deinit(void *p)
{
  if (unregister_binlog_relay_io_observer(&relay_io_observer, p))
    return 1;
  return 0;
}


struct Mysql_replication semi_sync_replica_plugin= {
  MYSQL_REPLICATION_INTERFACE_VERSION
};

/*
  Plugin library descriptor
*/
mysql_declare_plugin(semi_sync_replica)
{
  MYSQL_REPLICATION_PLUGIN,
  &semi_sync_replica_plugin,
  "rpl_semi_sync_replica",
  "He Zhenxing",
  "Semi-synchronous replication replica",
  PLUGIN_LICENSE_GPL,
  semi_sync_replica_plugin_init, /* Plugin Init */
  semi_sync_replica_plugin_deinit, /* Plugin Deinit */
  0x0100 /* 1.0 */,
  semi_sync_replica_status_vars,	/* status variables */
  semi_sync_replica_system_vars,	/* system variables */
  NULL,                         /* config options */
  0,                            /* flags */
}
mysql_declare_plugin_end;
