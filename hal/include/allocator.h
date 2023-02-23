// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Device Hardware Abstraction Layer device memory allocator.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HAL_ALLOCATOR_H_INCLUDED
#define HAL_ALLOCATOR_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <set>

#include "hal_types.h"

namespace hal {

struct allocator_t {
  struct block_t {
    block_t() : addr(0), is_free(true) {}

    // used for block ordering in the set
    bool operator<(const block_t &rhs) const { return addr < rhs.addr; }

    // block start address
    hal_addr_t addr;

    // true if this block is not yet allocated
    bool is_free;
  };

  // allocator constructed which will provide allocations within the memory
  // range specified.
  allocator_t(hal_addr_t base, hal_size_t size)
      : addr_lo(base), addr_hi(base + size) {
    assert(base != 0 && size != 0);
    reset();
  }

  // reset the allocator back to blank slate state.
  void reset() {
    blocks.clear();
    // create the initial free block
    block_t new_block;
    new_block.addr = addr_lo;
    new_block.is_free = true;
    blocks.insert(new_block);
  }

  // request a memory allocation of `size` bytes with the specified byte
  // alignment. `alignment` must be a power of two and non-zero.
  hal_addr_t alloc(hal_size_t size, hal_size_t alignment = 1) {
    assert(alignment != 0);
    assert((alignment & (alignment - 1)) == 0 &&
           "Alignment must be a power of two during alloc()");
    // zero size allocations are legal but have an effective size of 1 byte
    if (size == 0) {
      size = 1;
    }
    auto itt = blocks.begin();
    while (itt != blocks.end()) {
      // search for a free block
      auto next = std::next(itt);
      if (!itt->is_free) {
        itt = next;
        continue;
      }
      // compute the start point
      hal_addr_t start = block_end(itt);
      bool underflow = start < size;
      start -= size;
      start &= ~(hal_addr_t(alignment) - 1lu);
      // check if allocation will fit in this block
      if (start < itt->addr || underflow) {
        // this block does not have enough space, try next one
        itt = next;
        continue;
      }
      // allocation will not fit so create new block
      block_t new_block;
      new_block.addr = start;
      new_block.is_free = false;
      if (start == itt->addr) {
        // this block has just enough space and will not be divided.
        // remove free block so it can be replaced with a taken block.
        blocks.erase(itt);
      }
      // insert new block
      blocks.insert(new_block);
      return start;
    }
    // return nullptr
    return 0;
  }

  void free(hal_addr_t ptr) {
    // make free(0) an acceptable operation
    if (ptr == hal_nullptr) {
      return;
    }
    // create prototype block to search for
    block_t proto;
    proto.addr = ptr;
    proto.is_free = true;
    std::set<block_t>::iterator itt = blocks.find(proto);
    // check it is valid
    assert(itt != blocks.end() && "No block with this address found in free()");
    assert(itt->addr == ptr && "Found incorrect block in free()");
    assert(itt->is_free == false && "Block is already free in free()");
    if (itt != blocks.end()) {
      // construct a new free block in place of the original.
      // note that we cant simply change `is_free` as std::set iterators are
      // const due to the strict ordering.
      blocks.erase(itt);
      blocks.insert(proto);
      // run a pass to merge adjacent free blocks
      consolidate();
    }
  }

  // return the sum total of all free memory, note however that
  // memory fragmentation may impact the ability to allocate large chunks
  // even if the total memory is available.
  hal_size_t available() const {
    hal_size_t sum = 0;
    auto itt = blocks.begin();
    while (itt != blocks.end()) {
      if (itt->is_free) {
        sum += block_size(itt);
      }
      itt = std::next(itt);
    }
    return sum;
  }

 protected:
  // return the end address of this block
  hal_addr_t block_end(std::set<block_t>::iterator &itt) const {
    // find the end address
    hal_addr_t end = addr_hi;
    auto next = std::next(itt);
    if (next != blocks.end()) {
      end = next->addr;
    }
    return end;
  }

  // compute the number of bytes in a block
  hal_size_t block_size(std::set<block_t>::iterator &itt) const {
    // get the end address
    const hal_addr_t end = block_end(itt);
    // compute the block size
    const hal_size_t size = end - itt->addr;
    return size;
  }

  // merge adjacent free blocks
  void consolidate() {
    // loop over all blocks
    auto itt = blocks.begin();
    for (;;) {
      auto next = std::next(itt);
      if (next == blocks.end()) {
        // there are no more blocks to merge with
        break;
      }
      // we need two free adjacent blocks in order to merge them
      if (!itt->is_free || !next->is_free) {
        itt = next;
        continue;
      }
      // erase the next block to effectively give its space back to `itt`
      blocks.erase(next);
    }
  }

  // the valid address range to allocate within
  const hal_addr_t addr_lo;
  const hal_addr_t addr_hi;

  // the block list
  std::set<block_t> blocks;
};

}  // namespace hal

#endif  // HAL_ALLOCATOR_H_INCLUDED
