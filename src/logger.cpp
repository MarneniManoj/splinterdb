//
// Created by u1306224 on 11/28/22.
//
#include "platform.h"

#include "log.h"
#include "shard_log.h"
#include "io.h"
#include "allocator.h"
#include "rc_allocator.h"
#include "cache.h"
#include "clockcache.h"
#include "trunk.h"
#include "test.h"

#include "poison.h"

int write_to_wal(){

   log_handle        *logh;
   uint64             i;
   char               keybuffer[MAX_KEY_SIZE];
   slice              returned_key;
   message            returned_message;
   uint64             addr;
   uint64             magic;
   shard_log_iterator itor;
   iterator          *itorh = (iterator *)&itor;
   char               key_str[128];
   char               data_str[128];
   bool               at_end;
   merge_accumulator  msg;

   platform_heap_handle hh;
   platform_heap_id     hid;
   status = platform_heap_create(platform_get_module_id(), 1 * GiB, &hh, &hid);

   shard_log *log = TYPED_MALLOC(hid, log);
   log_handle        *logh;
   logh = (log_handle *)log;
   for (i = 0; i < num_entries; i++) {
      test_key(keybuffer, TEST_RANDOM, i, 0, 0, cfg->data_cfg->key_size, 0);
      generate_test_message(gen, i, &msg);
      slice skey = slice_create(1 + (i % cfg->data_cfg->key_size), keybuffer);
      log_write(logh, skey, merge_accumulator_to_message(&msg), i);
   }

}