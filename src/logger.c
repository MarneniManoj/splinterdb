//
// Created by u1306224 on 11/28/22.
//
#include "platform.h"

#include "log.h"
#include "logger.h"
#include "shard_log.h"
#include "io.h"
#include "allocator.h"
#include "rc_allocator.h"
#include "cache.h"
#include "clockcache.h"
#include "trunk.h"
#include "data_internal.h"

#include "poison.h"

int write_to_wal(){
   printf("****writing to wal ");
//   platform_status         status;
   log_handle        *logh;
//   uint64             i;
//   uint64             addr;
//   uint64             magic;
//   shard_log_iterator itor;
//   iterator          *itorh = (iterator *)&itor;
   platform_heap_handle hh;
   platform_heap_id     hid;
   platform_heap_create(platform_get_module_id(), 1 * GiB, &hh, &hid);
   shard_log *log = TYPED_MALLOC(hid, log);
   logh = (log_handle *)log;
   const char *fruit = "apple";
   const char *descr = "An apple a day keeps the doctor away!";

   slice key   = slice_create((size_t)strlen(fruit), fruit);
   slice value = slice_create((size_t)strlen(descr), descr);
   message msg1 = message_create(MESSAGE_TYPE_INSERT, value);
   log_write(logh, key, msg1, 0);
   return 0;

}

int read_log_entry(){
   platform_status        status;
   io_config              io_cfg;
   rc_allocator_config    al_cfg;
   clockcache_config      cache_cfg;
   shard_log_config       log_cfg;
   rc_allocator           al;
   platform_status    rc1;
   log_handle        *logh;

   slice              returned_key;
   message            returned_message;
   uint64             addr;
   uint64             magic;
   shard_log_iterator itor;
   iterator          *itorh = (iterator *)&itor;
   bool               at_end;
   merge_accumulator  msg;

   // Create a heap for io, allocator, cache and splinter
   platform_heap_handle hh;
   platform_heap_id     hid;
   status = platform_heap_create(platform_get_module_id(), 1 * GiB, &hh, &hid);
   platform_assert_status_ok(status);

   trunk_config *cfg = TYPED_MALLOC(hid, cfg);

//   status = test_parse_args(cfg,
//                            &data_cfg,
//                            &io_cfg,
//                            &al_cfg,
//                            &cache_cfg,
//                            &log_cfg,
//                            &seed,
//                            &gen,
//                            config_argc,
//                            config_argv);

//   if (!SUCCESS(status)) {
//      platform_error_log("log_test: failed to parse config: %s\n",
//                         platform_status_to_string(status));
//      /*
//       * Provided arguments but set things up incorrectly.
//       * Print usage so client can fix commandline.
//       */
//      usage(argv[0]);
//      rc = -1;
//      goto cleanup;
//   }

   platform_io_handle *io = TYPED_MALLOC(hid, io);
   platform_assert(io != NULL);
   status = io_handle_init(io, &io_cfg, hh, hid);
//   if (!SUCCESS(status)) {
//      rc = -1;
//      goto free_iohandle;
//   }

//   uint8 num_bg_threads[NUM_TASK_TYPES] = {0}; // no bg threads

//   status = test_init_task_system(
//      hid, io, &ts, cfg->use_stats, FALSE, num_bg_threads);
//   if (!SUCCESS(status)) {
//      platform_error_log("Failed to init splinter state: %s\n",
//                         platform_status_to_string(status));
//      rc = -1;
//      goto deinit_iohandle;
//   }

   status = rc_allocator_init(
      &al, &al_cfg, (io_handle *)io, hh, hid, platform_get_module_id());
   platform_assert_status_ok(status);

   clockcache *cc = TYPED_MALLOC(hid, cc);
   platform_assert(cc != NULL);
   status = clockcache_init(cc,
                            &cache_cfg,
                            (io_handle *)io,
                            (allocator *)&al,
                            "test",
                            hh,
                            hid,
                            platform_get_module_id());
   platform_assert_status_ok(status);
   shard_log *log = TYPED_MALLOC(hid, log);
   platform_assert(log != NULL);

   platform_assert(cc != NULL);
   rc1 = shard_log_init(log, (cache *)cc, &log_cfg);
   platform_assert_status_ok(rc1);
   logh = (log_handle *)log;

   addr  = log_addr(logh);
   magic = log_magic(logh);

   merge_accumulator_init(&msg, hid);


//   if (crash) {
//      clockcache_deinit(cc);
//      rc1 = clockcache_init(
//         cc, cache_cfg, io, al, "crashed", hh, hid, platform_get_module_id());
//      platform_assert_status_ok(rc1);
//   }

   rc1 = shard_log_iterator_init((cache *)cc, &log_cfg, hid, addr, magic, &itor);
   platform_assert_status_ok(rc1);
   itorh = (iterator *)&itor;

   iterator_at_end(itorh, &at_end);
   for (int i = 0; i < !at_end; i++) {
      iterator_get_curr(itorh, &returned_key, &returned_message);
      //      data_key_to_string(cfg->data_cfg, returned_key, key_str, 128);
      //      data_message_to_string(cfg->data_cfg, returned_message, data_str, 128);
      printf("log entry : key %s , value  %s\n", (char *)returned_key.data,  (char *)returned_message.data.data);
      iterator_advance(itorh);
      iterator_at_end(itorh, &at_end);
   }
   return 0;
}


int get_log_entry(){
   platform_status    rc;
   log_handle        *logh;
   uint64             addr;
   uint64             magic;
   shard_log_iterator itor;
   iterator          *itorh = (iterator *)&itor;
//   char               key_str[128];
//   char               data_str[128];
   slice              returned_key;
   message            returned_message;
   io_config              io_cfg;
   rc_allocator           al;
   rc_allocator_config    al_cfg;
   clockcache_config      cache_cfg;
   platform_heap_handle hh;
   platform_heap_id     hid;
   platform_heap_create(platform_get_module_id(), 1 * GiB, &hh, &hid);

   platform_io_handle *io = TYPED_MALLOC(hid, io);
   io_handle_init(io, &io_cfg, hh, hid);

   rc_allocator_init(
      &al, &al_cfg, (io_handle *)io, hh, hid, platform_get_module_id());

   clockcache *cc = TYPED_MALLOC(hid, cc);
   clockcache_init(cc,
                            &cache_cfg,
                            (io_handle *)io,
                            (allocator *)&al,
                            "test",
                            hh,
                            hid,
                            platform_get_module_id());

   shard_log *log = TYPED_MALLOC(hid, log);
   shard_log_config       log_cfg;

   rc = shard_log_init(log, (cache *)cc, &log_cfg);
   logh = (log_handle *)log;
   addr  = log_addr(logh);
   magic = log_magic(logh);

//   trunk_config *cfg = TYPED_MALLOC(hid, cfg);
   bool               at_end;
   rc = shard_log_iterator_init((cache *)cc, &log_cfg, hid, addr, magic, &itor);
   platform_assert_status_ok(rc);
   itorh = (iterator *)&itor;
   iterator_at_end(itorh, &at_end);
   for (int i = 0; i < !at_end; i++) {
      iterator_get_curr(itorh, &returned_key, &returned_message);
//      data_key_to_string(cfg->data_cfg, returned_key, key_str, 128);
//      data_message_to_string(cfg->data_cfg, returned_message, data_str, 128);
      printf("log entry : key %s , value  %s\n", (char *)returned_key.data,  (char *)returned_message.data.data);
      iterator_advance(itorh);
      iterator_at_end(itorh, &at_end);
   }
   return 0;
}


