// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Copyright (c) 2004, 2014, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/** @file ha_tera.h

  @brief
  The ha_example engine is a stubbed storage engine for example purposes only;
  it does nothing at this point. Its purpose is to provide a source
  code illustration of how to begin writing new storage engines; see also
  /storage/example/ha_example.cc.

  @note
  Please read ha_example.cc before reading this file.
  Reminder: The example storage engine implements all methods that are *required*
  to be implemented. For a full list of all methods that you can implement, see
  handler.h.

  @see
  /sql/handler.h and /storage/example/ha_example.cc
*/

#include "my_global.h"                   /* ulonglong */
#include "thr_lock.h"                    /* THR_LOCK, THR_LOCK_DATA */
#include "handler.h"                     /* handler */
#include "my_base.h"                     /* ha_rows */
#include <string>
#include <iostream>

namespace tera {
class Table;
class ResultStream;
}

class ha_tera_format;

/** @brief
  Tera_share is a class that will be shared among all open handlers.
  This example implements the minimum of what you will probably need.
*/
class Tera_share : public Handler_share {
public:
    THR_LOCK lock;
    tera::Table* table;
    Tera_share();
    ~Tera_share() {
        thr_lock_delete(&lock);
    }
};

/** @brief
  Class definition for the storage engine
*/
class ha_tera : public handler {
    THR_LOCK_DATA lock_; ///< MySQL lock
    Tera_share *share_; ///< Shared lock info
    Tera_share *get_share(const char* name); ///< Get the share
    tera::ResultStream* result_stream_;
    std::string last_key_;
    ha_tera_format* format_;

    int seek_row(const uchar *key, uint key_len);
    int read_row(uchar* buf);

public:
    ha_tera(handlerton *hton, TABLE_SHARE *table_arg);
    ~ha_tera();

    /** @brief
      The name that will be used for display purposes.
    */
    const char *table_type() const {
        return "TERA";
    }

    /** @brief
      The name of the index type that will be used for display.
      Don't implement this method unless you really have indexes.
    */
    const char *index_type(uint inx) {
        return "LSM";
    }

    /** @brief
      The file extensions.
    */
    const char **bas_ext() const;

    /** @brief
      This is a list of flags that indicate what functionality the storage engine
      implements. The current table flags are documented in handler.h
    */
    ulonglong table_flags() const {
        return HA_NO_TRANSACTIONS     | // not support txn
               HA_REC_NOT_IN_SEQ      | // support 'sort by', through position() and rnd_pos()
               HA_REQUIRE_PRIMARY_KEY | // must have a primary key
               HA_BINLOG_STMT_CAPABLE;
    }

    /** @brief
      This is a bitmap of flags that indicates how the storage engine
      implements indexes. The current index flags are documented in
      handler.h. If you do not implement indexes, just return zero here.

      @details
      part is the key part to check. First key part is 0.
      If all_parts is set, MySQL wants to know the flags for the combined
      index, up to and including 'part'.
    */
    ulong index_flags(uint inx, uint part, bool all_parts) const {
        return HA_READ_NEXT | HA_READ_ORDER | HA_READ_RANGE | HA_ONLY_WHOLE_INDEX;
    }

    /** @brief
      unireg.cc will call max_supported_record_length(), max_supported_keys(),
      max_supported_key_parts(), uint max_supported_key_length()
      to make sure that the storage engine can handle the data it is about to
      send. Return *real* limits of your storage engine here; MySQL will do
      min(your_limits, MySQL_limits) automatically.
    */
    uint max_supported_record_length() const {
        return HA_MAX_REC_LENGTH;
    }

    /** @brief
      unireg.cc will call this to make sure that the storage engine can handle
      the data it is about to send. Return *real* limits of your storage engine
      here; MySQL will do min(your_limits, MySQL_limits) automatically.

      @details
      There is no need to implement ..._key_... methods if your engine doesn't
      support indexes.
    */
    uint max_supported_keys() const {
        return 1;
    }

    /** @brief
      unireg.cc will call this to make sure that the storage engine can handle
      the data it is about to send. Return *real* limits of your storage engine
      here; MySQL will do min(your_limits, MySQL_limits) automatically.

      @details
      There is no need to implement ..._key_... methods if your engine doesn't
      support indexes.
    */
    uint max_supported_key_parts() const {
        return 1;
    }

    /** @brief
      unireg.cc will call this to make sure that the storage engine can handle
      the data it is about to send. Return *real* limits of your storage engine
      here; MySQL will do min(your_limits, MySQL_limits) automatically.

      @details
      There is no need to implement ..._key_... methods if your engine doesn't
      support indexes.
    */
    uint max_supported_key_length() const {
        return 1024;
    }

    /** @brief
      Called in test_quick_select to determine if indexes should be used.
    */
    virtual double scan_time() {
        return (double)(stats.records + stats.deleted) / 20.0 + 10;
    }

    /** @brief
      This method will never be called if you do not implement indexes.
    */
    virtual double read_time(uint, uint, ha_rows rows) {
        return (double)rows / 20.0 + 1;
    }

    /*
      Everything below are methods that we implement in ha_tera.cc.

      Most of these methods are not obligatory, skip them and
      MySQL will treat them as not implemented
    */
    /** @brief
      We implement this in ha_tera.cc; it's a required method.
    */
    int open(const char *name, int mode, uint test_if_locked); // required

    /** @brief
      We implement this in ha_tera.cc; it's a required method.
    */
    int close(void); // required

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int write_row(uchar *buf);

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int update_row(const uchar *old_data, uchar *new_data);

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int delete_row(const uchar *buf);

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int index_read(uchar *buf, const uchar *key, uint key_len, enum ha_rkey_function find_flag);

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int index_next(uchar *buf);

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int index_prev(uchar *buf);

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int index_first(uchar *buf);

    /** @brief
      We implement this in ha_tera.cc. It's not an obligatory method;
      skip it and and MySQL will treat it as not implemented.
    */
    int index_last(uchar *buf);

    /** @brief
      Unlike index_init(), rnd_init() can be called two consecutive times
      without rnd_end() in between (it only makes sense if scan=1). In this
      case, the second call should prepare for the new table scan (e.g if
      rnd_init() allocates the cursor, the second call should position the
      cursor to the start of the table; no need to deallocate and allocate
      it again. This is a required method.
    */
    int rnd_init(bool scan); //required
    int rnd_end();
    int rnd_next(uchar *buf); ///< required
    int rnd_pos(uchar *buf, uchar *pos); ///< required
    void position(const uchar *record); ///< required
    int info(uint); ///< required
    int extra(enum ha_extra_function operation);
    int external_lock(THD *thd, int lock_type); ///< required
    int delete_all_rows(void);
    int truncate();
    ha_rows records_in_range(uint inx, key_range *min_key, key_range *max_key);
    int delete_table(const char *from);
    int rename_table(const char * from, const char * to);
    int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info); ///< required

    THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to, enum thr_lock_type lock_type); ///< required
};
