/* Copyright (c) 2000, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#ifndef RPL_REPLICA_H
#define RPL_REPLICA_H

#include "my_global.h"
#include "my_thread.h"                     // my_start_routine
#include "mysql/psi/mysql_thread.h"        // mysql_mutex_t
#include "rpl_channel_service_interface.h" // enum_channel_type

class Log_event;
class Primary_info;
class Relay_log_info;
class THD;
typedef struct st_bitmap MY_BITMAP;
typedef struct st_lex_primary_info LEX_PRIMARY_INFO;
typedef struct st_list LIST;
typedef struct st_mysql MYSQL;
typedef struct st_net NET;
typedef struct struct_replica_connection LEX_REPLICA_CONNECTION;

typedef enum { REPLICA_THD_IO, REPLICA_THD_SQL, REPLICA_THD_WORKER } REPLICA_THD_TYPE;

/**
  PRIMARY_DELAY can be at most (1 << 31) - 1.
*/
#define PRIMARY_DELAY_MAX (0x7FFFFFFF)
#if INT_MAX < 0x7FFFFFFF
#error "don't support platforms where INT_MAX < 0x7FFFFFFF"
#endif

/**
  @defgroup Replication Replication
  @{

  @file
*/

/** 
   Some of defines are need in parser even though replication is not 
   compiled in (embedded).
*/

/**
   The maximum is defined as (ULONG_MAX/1000) with 4 bytes ulong
*/
#define REPLICA_MAX_HEARTBEAT_PERIOD 4294967

#ifdef HAVE_REPLICATION

#define REPLICA_NET_TIMEOUT  60

#define MAX_REPLICA_ERROR    2000

#define MTS_WORKER_UNDEF ((ulong) -1)
#define MTS_MAX_WORKERS  1024
#define MAX_REPLICA_RETRY_PAUSE 5

/* 
   When using tables to store the replica workers bitmaps,
   we use a BLOB field. The maximum size of a BLOB is:

   2^16-1 = 65535 bytes => (2^16-1) * 8 = 524280 bits
*/
#define MTS_MAX_BITS_IN_GROUP ((1L << 19) - 8) /* 524280 */

extern bool server_id_supplied;

/*****************************************************************************

  MySQL Replication

  Replication is implemented via two types of threads:

    I/O Thread - One of these threads is started for each primary server.
                 They maintain a connection to their primary server, read log
                 events from the primary as they arrive, and queues them into
                 a single, shared relay log file.  A Primary_info 
                 represents each of these threads.

    SQL Thread - One of these threads is started and reads from the relay log
                 file, executing each event.  A Relay_log_info 
                 represents this thread.

  Buffering in the relay log file makes it unnecessary to reread events from
  a primary server across a replica restart.  It also decouples the replica from
  the primary where long-running updates and event logging are concerned--ie
  it can continue to log new events while a slow query executes on the replica.

*****************************************************************************/

/*
  MUTEXES in replication:

  LOCK_msr_map: This is to lock the Multisource datastructure (msr_map).
  Generally it used to retrieve an mi from msr_map.It is used to SERIALIZE ALL
  administrative commands of replication: START REPLICA, STOP REPLICA, CHANGE
  PRIMARY, RESET REPLICA, end_replica() (when mysqld stops) [init_replica() does not
  need it it's called early]. Any of these commands holds the mutex from the
  start till the end. This thus protects us against a handful of deadlocks
  (consider start_replica_thread() which, when starting the I/O thread, releases
  mi->run_lock, keeps rli->run_lock, and tries to re-acquire mi->run_lock).

  Currently active_mi never moves (it's created at startup and deleted at
  shutdown, and not changed: it always points to the same Primary_info struct),
  because we don't have multiprimary. So for the moment, mi does not move, and
  mi->rli does not either.

  In Primary_info: run_lock, data_lock
  run_lock protects all information about the run state: replica_running, thd
  and the existence of the I/O thread (to stop/start it, you need this mutex).
  data_lock protects some moving members of the struct: counters (log name,
  position) and relay log (MYSQL_BIN_LOG object).

  In Relay_log_info: run_lock, data_lock
  see Primary_info
  However, note that run_lock does not protect
  Relay_log_info.run_state; that is protected by data_lock.

  In MYSQL_BIN_LOG: LOCK_log, LOCK_index of the binlog and the relay log
  LOCK_log: when you write to it. LOCK_index: when you create/delete a binlog
  (so that you have to update the .index file).

  The global_sid_lock must not be taken after LOCK_reset_gtid_table.

  ==== Order of acquisition ====

  Here, we list most major functions that acquire multiple locks.

  Notation: For each function, we list the locks it takes, in the
  order it takes them.  If a function holds lock A while taking lock
  B, then we write "A, B".  If a function locks A, unlocks A, then
  locks B, then we write "A | B".  If function F1 invokes function F2,
  then we write F2's name in parentheses in the list of locks for F1.

    show_primary_info:
      mi.data_lock, rli.data_lock, mi.err_lock, rli.err_lock

    stop_replica:
      LOCK_msr_map,
      ( mi.run_lock, thd.LOCK_thd_data
      | rli.run_lock, thd.LOCK_thd_data
      | relay.LOCK_log
      )

    start_replica:
      mi.run_lock, rli.run_lock, rli.data_lock, global_sid_lock->wrlock

    reset_logs:
      THD::LOCK_thd_data, .LOCK_log, .LOCK_index, global_sid_lock->wrlock

    purge_relay_logs:
      rli.data_lock, (relay.reset_logs) THD::LOCK_thd_data,
      relay.LOCK_log, relay.LOCK_index, global_sid_lock->wrlock

    reset_primary:
      (binlog.reset_logs) THD::LOCK_thd_data, binlog.LOCK_log,
      binlog.LOCK_index, global_sid_lock->wrlock, LOCK_reset_gtid_table

    reset_replica:
      mi.run_lock, rli.run_lock, (purge_relay_logs) rli.data_lock,
      THD::LOCK_thd_data, relay.LOCK_log, relay.LOCK_index,
      global_sid_lock->wrlock

    purge_logs:
      .LOCK_index, LOCK_thd_list, thd.linfo.lock

      [Note: purge_logs contains a known bug: LOCK_index should not be
      taken before LOCK_thd_list.  This implies that, e.g.,
      purge_primary_logs can deadlock with reset_primary.  However,
      although purge_first_log and reset_replica take locks in reverse
      order, they cannot deadlock because they both first acquire
      rli.data_lock.]

    purge_primary_logs, purge_primary_logs_before_date, purge:
      (binlog.purge_logs) binlog.LOCK_index, LOCK_thd_list, thd.linfo.lock

    purge_first_log:
      rli.data_lock, relay.LOCK_index, rli.log_space_lock,
      (relay.purge_logs) LOCK_thd_list, thd.linfo.lock

    MYSQL_BIN_LOG::new_file_impl:
      .LOCK_log, .LOCK_index,
      ( [ if binlog: LOCK_prep_xids ]
      | global_sid_lock->wrlock
      )

    rotate_relay_log:
      (relay.new_file_impl) relay.LOCK_log, relay.LOCK_index,
      global_sid_lock->wrlock

    kill_zombie_dump_threads:
      LOCK_thd_list, thd.LOCK_thd_data

    init_relay_log_pos:
      rli.data_lock, relay.log_lock

    rli_init_info:
      rli.data_lock,
      ( relay.log_lock
      | global_sid_lock->wrlock
      | (relay.open_binlog)
      | (init_relay_log_pos) rli.data_lock, relay.log_lock
      )

    change_primary:
      mi.run_lock, rli.run_lock, (init_relay_log_pos) rli.data_lock,
      relay.log_lock

    Sys_var_gtid_mode::global_update:
      gtid_mode_lock, LOCK_msr_map, binlog.LOCK_log, global_sid_lock

  So the DAG of lock acquisition order (not counting the buggy
  purge_logs) is, empirically:

    gtid_mode_lock, LOCK_msr_map, mi.run_lock, rli.run_lock,
      ( rli.data_lock,
        ( LOCK_thd_list,
          (
            ( binlog.LOCK_log, binlog.LOCK_index
            | relay.LOCK_log, relay.LOCK_index
            ),
            ( rli.log_space_lock | global_sid_lock->wrlock )
          | binlog.LOCK_log, binlog.LOCK_index, LOCK_prep_xids
          | thd.LOCK_data
          )
        | mi.err_lock, rli.err_lock
        )
      )
    )
    | mi.data_lock, rli.data_lock
*/

extern ulong primary_retry_count;
extern MY_BITMAP replica_error_mask;
extern char replica_skip_error_names[];
extern bool use_replica_mask;
extern char *replica_load_tmpdir;
extern char *primary_info_file, *relay_log_info_file;
extern char *opt_relay_logname, *opt_relaylog_index_name;
extern char *opt_binlog_index_name;
extern my_bool opt_skip_replica_start, opt_reckless_replica;
extern my_bool opt_log_replica_updates;
extern char *opt_replica_skip_errors;
extern ulonglong relay_log_space_limit;

extern const char *relay_log_index;
extern const char *relay_log_basename;

/*
  3 possible values for Primary_info::replica_running and
  Relay_log_info::replica_running.
  The values 0,1,2 are very important: to keep the diff small, I didn't
  substitute places where we use 0/1 with the newly defined symbols. So don't change
  these values.
  The same way, code is assuming that in Relay_log_info we use only values
  0/1.
  I started with using an enum, but
  enum_variable=1; is not legal so would have required many line changes.
*/
#define MYSQL_REPLICA_NOT_RUN         0
#define MYSQL_REPLICA_RUN_NOT_CONNECT 1
#define MYSQL_REPLICA_RUN_CONNECT     2

/*
  If the following is set, if first gives an error, second will be
  tried. Otherwise, if first fails, we fail.
*/
#define REPLICA_FORCE_ALL 4

/* @todo: see if you can change to int */
bool start_replica_cmd(THD* thd);
bool stop_replica_cmd(THD* thd);
bool change_primary_cmd(THD *thd);
int change_primary(THD* thd, Primary_info* mi, LEX_PRIMARY_INFO* lex_mi,
                  bool preserve_logs= false);
bool reset_replica_cmd(THD *thd);
bool show_replica_status_cmd(THD *thd);
bool flush_relay_logs_cmd(THD *thd);
bool is_any_replica_channel_running(int thread_mask,
                                  Primary_info* already_locked_mi=NULL);

bool flush_relay_logs(Primary_info *mi);
int reset_replica(THD *thd, Primary_info* mi, bool reset_all);
int reset_replica(THD *thd);
int init_replica();
int init_recovery(Primary_info* mi, const char** errmsg);
/**
  Call mi->init_info() and/or mi->rli->init_info(), which will read
  the replication configuration from repositories.

  This takes care of creating a transaction context in case table
  repository is needed.

  @param mi The Primary_info object to use.

  @param ignore_if_no_info If this is false, and the repository does
  not exist, it will be created. If this is true, and the repository
  does not exist, nothing is done.

  @param thread_mask Indicate which repositories will be initialized:
  if (thread_mask&REPLICA_IO)!=0, then mi->init_info is called; if
  (thread_mask&REPLICA_SQL)!=0, then mi->rli->init_info is called.

  @retval 0 Success
  @retval nonzero Error
*/
int global_init_info(Primary_info* mi, bool ignore_if_no_info, int thread_mask);
void end_info(Primary_info* mi);
int remove_info(Primary_info* mi);
int flush_primary_info(Primary_info* mi, bool force);
void add_replica_skip_errors(const char* arg);
void set_replica_skip_errors(char** replica_skip_errors_ptr);
int register_replica_on_primary(MYSQL* mysql);
int add_new_channel(Primary_info** mi, const char* channel,
                    enum_channel_type channel_type= REPLICA_REPLICATION_CHANNEL);
/**
  Terminates the replica threads according to the given mask.

  @param mi                the primary info repository
  @param thread_mask       the mask identifying which thread(s) to terminate
  @param stop_wait_timeout the timeout after which the method returns and error
  @param need_lock_term
          If @c false the lock will not be acquired before waiting on
          the condition. In this case, it is assumed that the calling
          function acquires the lock before calling this function.

  @return the operation status
    @retval 0    OK
    @retval ER_REPLICA_NOT_RUNNING
      The replica is already stopped
    @retval ER_STOP_REPLICA_SQL_THREAD_TIMEOUT
      There was a timeout when stopping the SQL thread
    @retval ER_STOP_REPLICA_IO_THREAD_TIMEOUT
      There was a timeout when stopping the IO thread
    @retval ER_ERROR_DURING_FLUSH_LOGS
      There was an error while flushing the log/repositories
*/
int terminate_replica_threads(Primary_info* mi, int thread_mask,
                            ulong stop_wait_timeout,
                            bool need_lock_term= true);
int start_replica_threads(bool need_lock_replica, bool wait_for_start,
			Primary_info* mi, int thread_mask);
int start_replica(THD *thd);
int stop_replica(THD *thd);
int start_replica(THD* thd,
                LEX_REPLICA_CONNECTION* connection_param,
                LEX_PRIMARY_INFO* primary_param,
                int thread_mask_input,
                Primary_info* mi,
                bool set_mts_settings,
                bool net_report);
int stop_replica(THD* thd, Primary_info* mi, bool net_report,
               bool for_one_channel=true);
/*
  cond_lock is usually same as start_lock. It is needed for the case when
  start_lock is 0 which happens if start_replica_thread() is called already
  inside the start_lock section, but at the same time we want a
  mysql_cond_wait() on start_cond, start_lock
*/
int start_replica_thread(
#ifdef HAVE_PSI_INTERFACE
                       PSI_thread_key thread_key,
#endif
                       my_start_routine h_func,
                       mysql_mutex_t *start_lock,
                       mysql_mutex_t *cond_lock,
                       mysql_cond_t *start_cond,
                       volatile uint *replica_running,
                       volatile ulong *replica_run_id,
                       Primary_info *mi);

/* retrieve table from primary and copy to replica*/
int fetch_primary_table(THD* thd, const char* db_name, const char* table_name,
		       Primary_info* mi, MYSQL* mysql, bool overwrite);

bool show_replica_status(THD* thd, Primary_info* mi);
bool show_replica_status(THD* thd);
bool rpl_primary_has_bug(const Relay_log_info *rli, uint bug_id, bool report,
                        bool (*pred)(const void *), const void *param);
bool rpl_primary_erroneous_autoinc(THD* thd);

const char *print_replica_db_safe(const char *db);
void skip_load_data_infile(NET* net);

void end_replica(); /* release replica threads */
void delete_replica_info_objects(); /* clean up replica threads data */
void clear_until_condition(Relay_log_info* rli);
void clear_replica_error(Relay_log_info* rli);
void lock_replica_threads(Primary_info* mi);
void unlock_replica_threads(Primary_info* mi);
void init_thread_mask(int* mask,Primary_info* mi,bool inverse);
void set_replica_thread_options(THD* thd);
void set_replica_thread_default_charset(THD *thd, Relay_log_info const *rli);
int apply_event_and_update_pos(Log_event* ev, THD* thd, Relay_log_info* rli);
int rotate_relay_log(Primary_info* mi);
int queue_event(Primary_info* mi,const char* buf, ulong event_len);

extern "C" void *handle_replica_io(void *arg);
extern "C" void *handle_replica_sql(void *arg);
bool net_request_file(NET* net, const char* fname);

extern bool volatile abort_loop;
extern Primary_info *active_mi;      /* active_mi  for multi-primary */
extern LIST primary_list;
extern my_bool replicate_same_server_id;

extern int disconnect_replica_event_count, abort_replica_event_count ;

/* the primary variables are defaults read from my.cnf or command line */
extern uint primary_port, primary_connect_retry, report_port;
extern char * primary_user, *primary_password, *primary_host;
extern char *primary_info_file, *relay_log_info_file, *report_user;
extern char *report_host, *report_password;

extern my_bool primary_ssl;
extern char *primary_ssl_ca, *primary_ssl_capath, *primary_ssl_cert;
extern char *primary_ssl_cipher, *primary_ssl_key;
       
int mts_recovery_groups(Relay_log_info *rli);
bool mts_checkpoint_routine(Relay_log_info *rli, ulonglong period,
                            bool force, bool need_data_lock);
bool sql_replica_killed(THD* thd, Relay_log_info* rli);
#endif /* HAVE_REPLICATION */

/* masks for start/stop operations on io and sql replica threads */
#define REPLICA_IO  1
#define REPLICA_SQL 2

/**
  @} (end of group Replication)
*/
#endif
