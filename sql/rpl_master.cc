/* Copyright (c) 2010, 2015, Oracle and/or its affiliates. All rights reserved.

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


#ifdef HAVE_REPLICATION
#include "rpl_primary.h"

#include "hash.h"                               // HASH
#include "m_string.h"                           // strmake
#include "auth_common.h"                        // check_global_access
#include "binlog.h"                             // mysql_bin_log
#include "debug_sync.h"                         // DEBUG_SYNC
#include "log.h"                                // sql_print_information
#include "mysqld_thd_manager.h"                 // Global_THD_manager
#include "rpl_binlog_sender.h"                  // Binlog_sender
#include "rpl_filter.h"                         // binlog_filter
#include "rpl_handler.h"                        // RUN_HOOK
#include "sql_class.h"                          // THD

#include "pfs_file_provider.h"
#include "mysql/psi/mysql_file.h"


int max_binlog_dump_events = 0; // unlimited
my_bool opt_sporadic_binlog_dump_fail = 0;

#define REPLICA_LIST_CHUNK 128
#define REPLICA_ERRMSG_SIZE (FN_REFLEN+64)
HASH replica_list;
extern TYPELIB binlog_checksum_typelib;


#define get_object(p, obj, msg) \
{\
  uint len; \
  if (p >= p_end) \
  { \
    my_error(ER_MALFORMED_PACKET, MYF(0)); \
    my_free(si); \
    return 1; \
  } \
  len= (uint)*p++;  \
  if (p + len > p_end || len >= sizeof(obj)) \
  {\
    errmsg= msg;\
    goto err; \
  }\
  strmake(obj,(char*) p,len); \
  p+= len; \
}\

extern "C" uint32
*replica_list_key(REPLICA_INFO* si, size_t *len,
		my_bool not_used __attribute__((unused)))
{
  *len = 4;
  return &si->server_id;
}

extern "C" void replica_info_free(void *s)
{
  my_free(s);
}

#ifdef HAVE_PSI_INTERFACE
static PSI_mutex_key key_LOCK_replica_list;

static PSI_mutex_info all_replica_list_mutexes[]=
{
  { &key_LOCK_replica_list, "LOCK_replica_list", PSI_FLAG_GLOBAL}
};

static void init_all_replica_list_mutexes(void)
{
  int count;

  count= array_elements(all_replica_list_mutexes);
  mysql_mutex_register("sql", all_replica_list_mutexes, count);
}
#endif /* HAVE_PSI_INTERFACE */

void init_replica_list()
{
#ifdef HAVE_PSI_INTERFACE
  init_all_replica_list_mutexes();
#endif

  my_hash_init(&replica_list, system_charset_info, REPLICA_LIST_CHUNK, 0, 0,
               (my_hash_get_key) replica_list_key,
               (my_hash_free_key) replica_info_free, 0,
               key_memory_REPLICA_INFO);
  mysql_mutex_init(key_LOCK_replica_list, &LOCK_replica_list, MY_MUTEX_INIT_FAST);
}

void end_replica_list()
{
  /* No protection by a mutex needed as we are only called at shutdown */
  if (my_hash_inited(&replica_list))
  {
    my_hash_free(&replica_list);
    mysql_mutex_destroy(&LOCK_replica_list);
  }
}

/**
  Register replica in 'replica_list' hash table.

  @return
    0	ok
  @return
    1	Error.   Error message sent to client
*/

int register_replica(THD* thd, uchar* packet, size_t packet_length)
{
  int res;
  REPLICA_INFO *si;
  uchar *p= packet, *p_end= packet + packet_length;
  const char *errmsg= "Wrong parameters to function register_replica";

  if (check_access(thd, REPL_REPLICA_ACL, any_db, NULL, NULL, 0, 0))
    return 1;
  if (!(si = (REPLICA_INFO*)my_malloc(key_memory_REPLICA_INFO,
                                    sizeof(REPLICA_INFO), MYF(MY_WME))))
    goto err2;

  /* 4 bytes for the server id */
  if (p + 4 > p_end)
  {
    my_error(ER_MALFORMED_PACKET, MYF(0));
    my_free(si);
    return 1;
  }

  thd->server_id= si->server_id= uint4korr(p);
  p+= 4;
  get_object(p,si->host, "Failed to register replica: too long 'report-host'");
  get_object(p,si->user, "Failed to register replica: too long 'report-user'");
  get_object(p,si->password, "Failed to register replica; too long 'report-password'");
  if (p+10 > p_end)
    goto err;
  si->port= uint2korr(p);
  p += 2;
  /* 
     We need to by pass the bytes used in the fake rpl_recovery_rank
     variable. It was removed in patch for BUG#13963. But this would 
     make a server with that patch unable to connect to an old primary.
     See: BUG#49259
  */
  p += 4;
  if (!(si->primary_id= uint4korr(p)))
    si->primary_id= server_id;
  si->thd= thd;

  mysql_mutex_lock(&LOCK_replica_list);
  unregister_replica(thd, false, false/*need_lock_replica_list=false*/);
  res= my_hash_insert(&replica_list, (uchar*) si);
  mysql_mutex_unlock(&LOCK_replica_list);
  return res;

err:
  my_free(si);
  my_message(ER_UNKNOWN_ERROR, errmsg, MYF(0)); /* purecov: inspected */
err2:
  return 1;
}

void unregister_replica(THD* thd, bool only_mine, bool need_lock_replica_list)
{
  if (thd->server_id)
  {
    if (need_lock_replica_list)
      mysql_mutex_lock(&LOCK_replica_list);
    else
      mysql_mutex_assert_owner(&LOCK_replica_list);

    REPLICA_INFO* old_si;
    if ((old_si = (REPLICA_INFO*)my_hash_search(&replica_list,
                                              (uchar*)&thd->server_id, 4)) &&
	(!only_mine || old_si->thd == thd))
    my_hash_delete(&replica_list, (uchar*)old_si);

    if (need_lock_replica_list)
      mysql_mutex_unlock(&LOCK_replica_list);
  }
}


/**
  Execute a SHOW REPLICA HOSTS statement.

  @param thd Pointer to THD object for the client thread executing the
  statement.

  @retval FALSE success
  @retval TRUE failure
*/
bool show_replica_hosts(THD* thd)
{
  List<Item> field_list;
  Protocol *protocol= thd->get_protocol();
  DBUG_ENTER("show_replica_hosts");

  field_list.push_back(new Item_return_int("Server_id", 10,
					   MYSQL_TYPE_LONG));
  field_list.push_back(new Item_empty_string("Host", 20));
  if (opt_show_replica_auth_info)
  {
    field_list.push_back(new Item_empty_string("User",20));
    field_list.push_back(new Item_empty_string("Password",20));
  }
  field_list.push_back(new Item_return_int("Port", 7, MYSQL_TYPE_LONG));
  field_list.push_back(new Item_return_int("Primary_id", 10,
					   MYSQL_TYPE_LONG));
  field_list.push_back(new Item_empty_string("Replica_UUID", UUID_LENGTH));

  if (thd->send_result_metadata(&field_list,
                                Protocol::SEND_NUM_ROWS | Protocol::SEND_EOF))
    DBUG_RETURN(TRUE);

  mysql_mutex_lock(&LOCK_replica_list);

  for (uint i = 0; i < replica_list.records; ++i)
  {
    REPLICA_INFO* si = (REPLICA_INFO*) my_hash_element(&replica_list, i);
    protocol->start_row();
    protocol->store((uint32) si->server_id);
    protocol->store(si->host, &my_charset_bin);
    if (opt_show_replica_auth_info)
    {
      protocol->store(si->user, &my_charset_bin);
      protocol->store(si->password, &my_charset_bin);
    }
    protocol->store((uint32) si->port);
    protocol->store((uint32) si->primary_id);

    /* get replica's UUID */
    String replica_uuid;
    if (get_replica_uuid(si->thd, &replica_uuid))
      protocol->store(replica_uuid.c_ptr_safe(), &my_charset_bin);
    if (protocol->end_row())
    {
      mysql_mutex_unlock(&LOCK_replica_list);
      DBUG_RETURN(TRUE);
    }
  }
  mysql_mutex_unlock(&LOCK_replica_list);
  my_eof(thd);
  DBUG_RETURN(FALSE);
}

/**
  If there are less than BYTES bytes left to read in the packet,
  report error.
*/
#define CHECK_PACKET_SIZE(BYTES)                                        \
  do {                                                                  \
    if (packet_bytes_todo < BYTES)                                      \
      goto error_malformed_packet;                                      \
  } while (0)

/**
  Auxiliary macro used to define READ_INT and READ_STRING.

  Check that there are at least BYTES more bytes to read, then read
  the bytes using the given DECODER, then advance the reading
  position.
*/
#define READ(DECODE, BYTES)                                             \
  do {                                                                  \
    CHECK_PACKET_SIZE(BYTES);                                           \
    DECODE;                                                             \
    packet_position+= BYTES;                                            \
    packet_bytes_todo-= BYTES;                                          \
  } while (0)


/**
  Check that there are at least BYTES more bytes to read, then read
  the bytes and decode them into the given integer VAR, then advance
  the reading position.
*/
#define READ_INT(VAR, BYTES)                                            \
  READ(VAR= uint ## BYTES ## korr(packet_position), BYTES)

/**
  Check that there are at least BYTES more bytes to read and that
  BYTES+1 is not greater than BUFFER_SIZE, then read the bytes into
  the given variable VAR, then advance the reading position.
*/
#define READ_STRING(VAR, BYTES, BUFFER_SIZE)                            \
  do {                                                                  \
    if (BUFFER_SIZE <= BYTES)                                           \
      goto error_malformed_packet;                                      \
    READ(memcpy(VAR, packet_position, BYTES), BYTES);                   \
    VAR[BYTES]= '\0';                                                   \
  } while (0)


bool com_binlog_dump(THD *thd, char *packet, size_t packet_length)
{
  DBUG_ENTER("com_binlog_dump");
  ulong pos;
  String replica_uuid;
  ushort flags= 0;
  const uchar* packet_position= (uchar *) packet;
  size_t packet_bytes_todo= packet_length;

  thd->status_var.com_other++;
  thd->enable_slow_log= opt_log_slow_admin_statements;
  if (check_global_access(thd, REPL_REPLICA_ACL))
    DBUG_RETURN(false);

  /*
    4 bytes is too little, but changing the protocol would break
    compatibility.  This has been fixed in the new protocol. @see
    com_binlog_dump_gtid().
  */
  READ_INT(pos, 4);
  READ_INT(flags, 2);
  READ_INT(thd->server_id, 4);

  DBUG_PRINT("info", ("pos=%lu flags=%d server_id=%d", pos, flags, thd->server_id));

  get_replica_uuid(thd, &replica_uuid);
  kill_zombie_dump_threads(&replica_uuid);

  query_logger.general_log_print(thd, thd->get_command(), "Log: '%s'  Pos: %ld",
                                 packet + 10, (long) pos);
  mysql_binlog_send(thd, thd->mem_strdup(packet + 10), (my_off_t) pos, NULL, flags);

  unregister_replica(thd, true, true/*need_lock_replica_list=true*/);
  /*  fake COM_QUIT -- if we get here, the thread needs to terminate */
  DBUG_RETURN(true);

error_malformed_packet:
  my_error(ER_MALFORMED_PACKET, MYF(0));
  DBUG_RETURN(true);
}


bool com_binlog_dump_gtid(THD *thd, char *packet, size_t packet_length)
{
  DBUG_ENTER("com_binlog_dump_gtid");
  /*
    Before going GA, we need to make this protocol extensible without
    breaking compatitibilty. /Alfranio.
  */
  String replica_uuid;
  ushort flags= 0;
  uint32 data_size= 0;
  uint64 pos= 0;
  char name[FN_REFLEN + 1];
  uint32 name_size= 0;
  char* gtid_string= NULL;
  const uchar* packet_position= (uchar *) packet;
  size_t packet_bytes_todo= packet_length;
  Sid_map sid_map(NULL/*no sid_lock because this is a completely local object*/);
  Gtid_set replica_gtid_executed(&sid_map);

  thd->status_var.com_other++;
  thd->enable_slow_log= opt_log_slow_admin_statements;
  if (check_global_access(thd, REPL_REPLICA_ACL))
    DBUG_RETURN(false);

  READ_INT(flags,2);
  READ_INT(thd->server_id, 4);
  READ_INT(name_size, 4);
  READ_STRING(name, name_size, sizeof(name));
  READ_INT(pos, 8);
  DBUG_PRINT("info", ("pos=%llu flags=%d server_id=%d", pos, flags, thd->server_id));
  READ_INT(data_size, 4);
  CHECK_PACKET_SIZE(data_size);
  if (replica_gtid_executed.add_gtid_encoding(packet_position, data_size) !=
      RETURN_STATUS_OK)
    DBUG_RETURN(true);
  replica_gtid_executed.to_string(&gtid_string);
  DBUG_PRINT("info", ("Replica %d requested to read %s at position %llu gtid set "
                      "'%s'.", thd->server_id, name, pos, gtid_string));

  get_replica_uuid(thd, &replica_uuid);
  kill_zombie_dump_threads(&replica_uuid);
  query_logger.general_log_print(thd, thd->get_command(),
                                 "Log: '%s' Pos: %llu GTIDs: '%s'",
                                 name, pos, gtid_string);
  my_free(gtid_string);
  mysql_binlog_send(thd, name, (my_off_t) pos, &replica_gtid_executed, flags);

  unregister_replica(thd, true, true/*need_lock_replica_list=true*/);
  /*  fake COM_QUIT -- if we get here, the thread needs to terminate */
  DBUG_RETURN(true);

error_malformed_packet:
  my_error(ER_MALFORMED_PACKET, MYF(0));
  DBUG_RETURN(true);
}

void mysql_binlog_send(THD* thd, char* log_ident, my_off_t pos,
                       Gtid_set* replica_gtid_executed, uint32 flags)
{
  Binlog_sender sender(thd, log_ident, pos, replica_gtid_executed, flags);

  sender.run();
}

/**
  An auxiliary function extracts replica UUID.

  @param[in]    thd  THD to access a user variable
  @param[out]   value String to return UUID value.

  @return       if success value is returned else NULL is returned.
*/
String *get_replica_uuid(THD *thd, String *value)
{
  uchar name[]= "replica_uuid";

  if (value == NULL)
    return NULL;

  /* Protects thd->user_vars. */
  mysql_mutex_lock(&thd->LOCK_thd_data);

  user_var_entry *entry=
    (user_var_entry*) my_hash_search(&thd->user_vars, name, sizeof(name)-1);
  if (entry && entry->length() > 0)
  {
    value->copy(entry->ptr(), entry->length(), NULL);
    mysql_mutex_unlock(&thd->LOCK_thd_data);
    return value;
  }

  mysql_mutex_unlock(&thd->LOCK_thd_data);
  return NULL;
}

/**
  Callback function used by kill_zombie_dump_threads() function to
  to find zombie dump thread from the thd list.

  @note It acquires LOCK_thd_data mutex when it finds matching thd.
  It is the responsibility of the caller to release this mutex.
*/
class Find_zombie_dump_thread : public Find_THD_Impl
{
public:
  Find_zombie_dump_thread(char* value): m_replica_uuid(value) {}
  virtual bool operator()(THD *thd)
  {
    if (thd != current_thd && (thd->get_command() == COM_BINLOG_DUMP ||
                               thd->get_command() == COM_BINLOG_DUMP_GTID))
    {
      String tmp_uuid;
      if (get_replica_uuid(thd, &tmp_uuid) != NULL &&
          !strncmp(m_replica_uuid, tmp_uuid.c_ptr(), UUID_LENGTH))
      {
        mysql_mutex_lock(&thd->LOCK_thd_data);
        return true;
      }
    }
    return false;
  }
private:
  char* m_replica_uuid;
};

/*

  Kill all Binlog_dump threads which previously talked to the same replica
  ("same" means with the same server id). Indeed, if the replica stops, if the
  Binlog_dump thread is waiting (mysql_cond_wait) for binlog update, then it
  will keep existing until a query is written to the binlog. If the primary is
  idle, then this could last long, and if the replica reconnects, we could have 2
  Binlog_dump threads in SHOW PROCESSLIST, until a query is written to the
  binlog. To avoid this, when the replica reconnects and sends COM_BINLOG_DUMP,
  the primary kills any existing thread with the replica's server id (if this id is
  not zero; it will be true for real replicas, but false for mysqlbinlog when it
  sends COM_BINLOG_DUMP to get a remote binlog dump).

  SYNOPSIS
    kill_zombie_dump_threads()
    replica_uuid      the replica's UUID

*/

void kill_zombie_dump_threads(String *replica_uuid)
{
  if (replica_uuid->length() == 0)
    return;
  DBUG_ASSERT(replica_uuid->length() == UUID_LENGTH);

  Find_zombie_dump_thread find_zombie_dump_thread(replica_uuid->c_ptr());
  THD *tmp= Global_THD_manager::get_instance()->
                                find_thd(&find_zombie_dump_thread);
  if (tmp)
  {
    /*
      Here we do not call kill_one_thread() as
      it will be slow because it will iterate through the list
      again. We just to do kill the thread ourselves.
    */
    if (log_warnings > 1)
      sql_print_information("While initializing dump thread for replica with "
                            "UUID <%s>, found a zombie dump thread with "
                            "the same UUID. Primary is killing the zombie dump "
                            "thread.", replica_uuid->c_ptr());
    tmp->duplicate_replica_uuid= true;
    tmp->awake(THD::KILL_QUERY);
    mysql_mutex_unlock(&tmp->LOCK_thd_data);
  }
}


/**
  Execute a RESET PRIMARY statement.

  @param thd Pointer to THD object of the client thread executing the
  statement.

  @retval false success
  @retval true error
*/
bool reset_primary(THD* thd)
{
  bool ret= false;
  if (mysql_bin_log.is_open())
  {
    /*
      mysql_bin_log.reset_logs will delete the binary logs *and* clear
      gtid_state.  It is important to do both these operations from
      within reset_logs, since the operations can then use the same
      lock.  I.e., if we would remove the call to gtid_state->clear
      from reset_logs and call gtid_state->clear explicitly from this
      function instead, it would be possible for a concurrent thread
      to commit between the point where the binary log was removed and
      the point where the gtid_executed table is cleared. This would
      lead to an inconsistent state.
    */
    ret= mysql_bin_log.reset_logs(thd);
  }
  else
  {
    global_sid_lock->wrlock();
    ret= (gtid_state->clear(thd) != 0);
    global_sid_lock->unlock();
  }

  /*
    Only run after_reset_primary hook, when all reset operations preceding this
    have succeeded.
  */
  if (!ret)
    (void) RUN_HOOK(binlog_transmit, after_reset_primary, (thd, 0 /* flags */));
  return ret;
 }


/**
  Execute a SHOW PRIMARY STATUS statement.

  @param thd Pointer to THD object for the client thread executing the
  statement.

  @retval false success
  @retval true failure
*/
bool show_primary_status(THD* thd)
{
  Protocol *protocol= thd->get_protocol();
  char* gtid_set_buffer= NULL;
  int gtid_set_size= 0;
  List<Item> field_list;

  DBUG_ENTER("show_binlog_info");

  global_sid_lock->wrlock();
  const Gtid_set* gtid_set= gtid_state->get_executed_gtids();
  if ((gtid_set_size= gtid_set->to_string(&gtid_set_buffer)) < 0)
  {
    global_sid_lock->unlock();
    my_eof(thd);
    my_free(gtid_set_buffer);
    DBUG_RETURN(true);
  }
  global_sid_lock->unlock();

  field_list.push_back(new Item_empty_string("File", FN_REFLEN));
  field_list.push_back(new Item_return_int("Position",20,
					   MYSQL_TYPE_LONGLONG));
  field_list.push_back(new Item_empty_string("Binlog_Do_DB",255));
  field_list.push_back(new Item_empty_string("Binlog_Ignore_DB",255));
  field_list.push_back(new Item_empty_string("Executed_Gtid_Set",
                                             gtid_set_size));

  if (thd->send_result_metadata(&field_list,
                                Protocol::SEND_NUM_ROWS | Protocol::SEND_EOF))
  {
    my_free(gtid_set_buffer);
    DBUG_RETURN(true);
  }
  protocol->start_row();

  if (mysql_bin_log.is_open())
  {
    LOG_INFO li;
    mysql_bin_log.get_current_log(&li);
    size_t dir_len = dirname_length(li.log_file_name);
    protocol->store(li.log_file_name + dir_len, &my_charset_bin);
    protocol->store((ulonglong) li.pos);
    store(protocol, binlog_filter->get_do_db());
    store(protocol, binlog_filter->get_ignore_db());
    protocol->store(gtid_set_buffer, &my_charset_bin);
    if (protocol->end_row())
    {
      my_free(gtid_set_buffer);
      DBUG_RETURN(true);
    }
  }
  my_eof(thd);
  my_free(gtid_set_buffer);
  DBUG_RETURN(false);
}


/**
  Execute a SHOW BINARY LOGS statement.

  @param thd Pointer to THD object for the client thread executing the
  statement.

  @retval FALSE success
  @retval TRUE failure
*/
bool show_binlogs(THD* thd)
{
  IO_CACHE *index_file;
  LOG_INFO cur;
  File file;
  char fname[FN_REFLEN];
  List<Item> field_list;
  size_t length;
  size_t cur_dir_len;
  Protocol *protocol= thd->get_protocol();
  DBUG_ENTER("show_binlogs");

  if (!mysql_bin_log.is_open())
  {
    my_error(ER_NO_BINARY_LOGGING, MYF(0));
    DBUG_RETURN(TRUE);
  }

  field_list.push_back(new Item_empty_string("Log_name", 255));
  field_list.push_back(new Item_return_int("File_size", 20,
                                           MYSQL_TYPE_LONGLONG));
    if (thd->send_result_metadata(&field_list, Protocol::SEND_NUM_ROWS |
                                    Protocol::SEND_EOF))
    DBUG_RETURN(TRUE);
  
  mysql_mutex_lock(mysql_bin_log.get_log_lock());
  DEBUG_SYNC(thd, "show_binlogs_after_lock_log_before_lock_index");
  mysql_bin_log.lock_index();
  index_file=mysql_bin_log.get_index_file();
  
  mysql_bin_log.raw_get_current_log(&cur); // dont take mutex
  mysql_mutex_unlock(mysql_bin_log.get_log_lock()); // lockdep, OK
  
  cur_dir_len= dirname_length(cur.log_file_name);

  reinit_io_cache(index_file, READ_CACHE, (my_off_t) 0, 0, 0);

  /* The file ends with EOF or empty line */
  while ((length=my_b_gets(index_file, fname, sizeof(fname))) > 1)
  {
    size_t dir_len;
    ulonglong file_length= 0;                   // Length if open fails
    fname[--length] = '\0';                     // remove the newline

    protocol->start_row();
    dir_len= dirname_length(fname);
    length-= dir_len;
    protocol->store(fname + dir_len, length, &my_charset_bin);

    if (!(strncmp(fname+dir_len, cur.log_file_name+cur_dir_len, length)))
      file_length= cur.pos;  /* The active log, use the active position */
    else
    {
      /* this is an old log, open it and find the size */
      if ((file= mysql_file_open(key_file_binlog,
                                 fname, O_RDONLY | O_SHARE | O_BINARY,
                                 MYF(0))) >= 0)
      {
        file_length= (ulonglong) mysql_file_seek(file, 0L, MY_SEEK_END, MYF(0));
        mysql_file_close(file, MYF(0));
      }
    }
    protocol->store(file_length);
    if (protocol->end_row())
    {
      DBUG_PRINT("info", ("stopping dump thread because protocol->write failed at line %d", __LINE__));
      goto err;
    }
  }
  if(index_file->error == -1)
    goto err;
  mysql_bin_log.unlock_index();
  my_eof(thd);
  DBUG_RETURN(FALSE);

err:
  mysql_bin_log.unlock_index();
  DBUG_RETURN(TRUE);
}

#endif /* HAVE_REPLICATION */
