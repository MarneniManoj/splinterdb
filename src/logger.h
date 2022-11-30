#include "platform.h"

#include "log.h"
#include "shard_log.h"
#include "io.h"
#include "allocator.h"
#include "rc_allocator.h"
#include "cache.h"
#include "clockcache.h"
#include "trunk.h"

#include "poison.h"
int write_to_wal();
int get_log_entry();
int read_log_entry();
void print_log_entry(log_handle *logh);