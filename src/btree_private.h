// Copyright 2021 VMware, Inc.
// SPDX-License-Identifier: Apache-2.0

/*
 * btree_private.h --
 *
 * This file contains the private interfaces for dynamic b-trees/memtables.
 * These definitions are provided here so that they can be shared by the
 * source and test modules.
 */
#ifndef __BTREE_PRIVATE_H__
#define __BTREE_PRIVATE_H__

#include "splinterdb/platform_public.h"
#include "splinterdb/data.h"
#include "util.h"
#include "btree.h"

typedef uint16 table_index; //  So we can make this bigger for bigger nodes.
typedef uint16 node_offset; //  So we can make this bigger for bigger nodes.
typedef node_offset table_entry;
typedef uint16      inline_key_size;
typedef uint16      inline_message_size;

/* **********************
 * Node headers
 * *********************
 */
struct PACKED btree_hdr {
   uint64      next_addr;
   uint64      next_extent_addr;
   uint64      generation;
   uint8       height;
   node_offset next_entry;
   table_index num_entries;
   table_entry offsets[];
};

/* **********************************
 * Node entries
 * *********************************
 */
typedef struct PACKED index_entry {
   btree_pivot_data pivot_data;
   inline_key_size  key_size;
   char             key[];
} index_entry;

_Static_assert(sizeof(index_entry)
                  == sizeof(uint64) + 3 * sizeof(uint32)
                        + sizeof(inline_key_size),
               "index_entry has wrong size");
_Static_assert(offsetof(index_entry, key) == sizeof(index_entry),
               "index_entry key has wrong offset");

typedef struct PACKED leaf_entry {
   inline_key_size     key_size;
   inline_message_size message_size;
   char                key_and_message[];
} leaf_entry;

_Static_assert(sizeof(leaf_entry)
                  == sizeof(inline_key_size) + sizeof(inline_message_size),
               "leaf_entry has wrong size");
_Static_assert(offsetof(leaf_entry, key_and_message) == sizeof(leaf_entry),
               "leaf_entry key_and_data has wrong offset");

typedef struct leaf_incorporate_spec {
   slice key;
   int64 idx;
   bool  was_found;
   union {
      /* "was_found" is the tag on this union. */
      slice           new_message;    /* was_found == FALSE */
      writable_buffer merged_message; /* was_found == TRUE */
   } msg;
} leaf_incorporate_spec;

platform_status
btree_create_leaf_incorporate_spec(const btree_config    *cfg,
                                   platform_heap_id       heap_id,
                                   btree_hdr             *hdr,
                                   slice                  key,
                                   slice                  message,
                                   leaf_incorporate_spec *spec);

bool
btree_try_perform_leaf_incorporate_spec(const btree_config          *cfg,
                                        btree_hdr                   *hdr,
                                        const leaf_incorporate_spec *spec,
                                        uint64 *generation);

/*
 * This structure is intended to capture all the decisions in a leaf split.
 * That way, we can have a single function that defines the entire policy,
 * separate from the code that executes the policy (possibly as several steps
 * for concurrency reasons).
 */
typedef struct leaf_splitting_plan {
   uint64 split_idx;         // keys with idx < split_idx go left
   bool insertion_goes_left; // does the key to be inserted go to the left child
} leaf_splitting_plan;

/*
 * ************************************************************************
 * External function prototypes: Declare these first, as some inine static
 * functions defined below may call these extern functions.
 * ************************************************************************
 */
bool
btree_set_index_entry(const btree_config *cfg,
                      btree_hdr          *hdr,
                      table_index         k,
                      slice               new_pivot_key,
                      uint64              new_addr,
                      int64               kv_pairs,
                      int64               key_bytes,
                      int64               message_bytes);

bool
btree_set_leaf_entry(const btree_config *cfg,
                     btree_hdr          *hdr,
                     table_index         k,
                     slice               new_key,
                     slice               new_message);

void
btree_defragment_leaf(const btree_config *cfg, // IN
                      btree_scratch      *scratch,
                      btree_hdr          *hdr,
                      int64               omit_idx); // IN

void
btree_defragment_index(const btree_config *cfg, // IN
                       btree_scratch      *scratch,
                       btree_hdr          *hdr); // IN

int64
btree_find_pivot(const btree_config *cfg,
                 const btree_hdr    *hdr,
                 slice               key,
                 bool               *found);

leaf_splitting_plan
btree_build_leaf_splitting_plan(const btree_config          *cfg, // IN
                                const btree_hdr             *hdr,
                                const leaf_incorporate_spec *spec); // IN

/*
 * ***********************************************************
 * Inline accessor functions for different private structure.
 * ***********************************************************
 */
static inline void
btree_init_hdr(const btree_config *cfg, btree_hdr *hdr)
{
   ZERO_CONTENTS(hdr);
   hdr->next_entry = cfg->page_size;
}

static inline uint64
btree_page_size(const btree_config *cfg)
{
   return cfg->page_size;
}

static inline uint64
sizeof_index_entry(const index_entry *entry)
{
   return sizeof(*entry) + entry->key_size;
}

static inline uint64
sizeof_leaf_entry(const leaf_entry *entry)
{
   return sizeof(*entry) + entry->key_size + entry->message_size;
}

static inline slice
index_entry_key_slice(const index_entry *entry)
{
   return slice_create(entry->key_size, entry->key);
}

static inline uint64
index_entry_child_addr(const index_entry *entry)
{
   return entry->pivot_data.child_addr;
}

static inline slice
leaf_entry_key_slice(leaf_entry *entry)
{
   return slice_create(entry->key_size, entry->key_and_message);
}

static inline slice
leaf_entry_message_slice(leaf_entry *entry)
{
   return slice_create(entry->message_size,
                       entry->key_and_message + entry->key_size);
}

static inline leaf_entry *
btree_get_leaf_entry(const btree_config *cfg,
                     const btree_hdr    *hdr,
                     table_index         k)
{
   /* Ensure that the kth entry's header is after the end of the table and
    * before the end of the page.
    */
   debug_assert(diff_ptr(hdr, &hdr->offsets[hdr->num_entries])
                <= hdr->offsets[k]);
   debug_assert(hdr->offsets[k] + sizeof(leaf_entry) <= btree_page_size(cfg));
   leaf_entry *entry =
      (leaf_entry *)const_pointer_byte_offset(hdr, hdr->offsets[k]);
   debug_assert(hdr->offsets[k] + sizeof_leaf_entry(entry)
                <= btree_page_size(cfg));
   return entry;
}

static inline slice
btree_get_tuple_key(const btree_config *cfg,
                    const btree_hdr    *hdr,
                    table_index         k)
{
   return leaf_entry_key_slice(btree_get_leaf_entry(cfg, hdr, k));
}

static inline slice
btree_get_tuple_message(const btree_config *cfg,
                        const btree_hdr    *hdr,
                        table_index         k)
{
   return leaf_entry_message_slice(btree_get_leaf_entry(cfg, hdr, k));
}

static inline index_entry *
btree_get_index_entry(const btree_config *cfg,
                      const btree_hdr    *hdr,
                      table_index         k)
{
   /* Ensure that the kth entry's header is after the end of the table and
    * before the end of the page.
    */
   debug_assert(diff_ptr(hdr, &hdr->offsets[hdr->num_entries])
                <= hdr->offsets[k]);
   debug_assert(hdr->offsets[k] + sizeof(index_entry) <= btree_page_size(cfg),
                "k=%d, offsets[k]=%d, sizeof(index_entry)=%lu"
                ", btree_page_size=%lu.",
                k,
                hdr->offsets[k],
                sizeof(index_entry),
                btree_page_size(cfg));

   index_entry *entry =
      (index_entry *)const_pointer_byte_offset(hdr, hdr->offsets[k]);

   /* Now ensure that the entire entry fits in the page. */
   debug_assert(hdr->offsets[k] + sizeof_index_entry(entry)
                   <= btree_page_size(cfg),
                "Offsets entry at index k=%d does not fit in the page."
                " offsets[k]=%d, sizeof_index_entry()=%lu"
                ", btree_page_size=%lu.",
                k,
                hdr->offsets[k],
                sizeof_index_entry(entry),
                btree_page_size(cfg));
   return entry;
}

static inline slice
btree_get_pivot(const btree_config *cfg, const btree_hdr *hdr, table_index k)
{
   return index_entry_key_slice(btree_get_index_entry(cfg, hdr, k));
}

static inline uint64
btree_get_child_addr(const btree_config *cfg,
                     const btree_hdr    *hdr,
                     table_index         k)
{
   return index_entry_child_addr(btree_get_index_entry(cfg, hdr, k));
}

#endif // __BTREE_PRIVATE_H__