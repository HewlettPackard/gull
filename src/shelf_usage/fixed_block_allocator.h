/*
  modified from the fixed block allocator implementation from Mark
 */

#ifndef _NVMM_FIXED_BLOCK_ALLOCATOR_H_
#define _NVMM_FIXED_BLOCK_ALLOCATOR_H_

#include <stddef.h>
#include "shelf_usage/smart_shelf.h"

namespace nvmm {

class FixedBlockAllocator {
public:
    // may throw runtime_error
    FixedBlockAllocator(void *addr, size_t block_size,
                        size_t user_metadata_size,
                        size_t initial_pool_size, size_t max_pool_size);
    virtual ~FixedBlockAllocator() {}


    void*   user_metadata();
    size_t  user_metadata_size();

    size_t  size();        // size of our underlying shelf
    size_t  block_size();
    // maximum number of blocks that can be allocated at once:
    int64_t max_blocks();

    SmartShelf_& get_underlying_shelf();    

    // returns 0 if no blocks are currently available
    Offset alloc();
    // [unsafe_]free(0) is a no-op
    void     free       (Offset block);
    // requires all writes to block block have already been persisted
    void     unsafe_free(Offset block);


    // converting to and from local-address-space pointers
    void*    from_Offset(Offset p); 
    Offset to_Offset  (void*    p);

    // shortcut for from_Offset; does not work well on FixedBlockAllocator*:
    //   need (*fba)[ptr] for that case
    void* operator[](Offset p) { return from_Offset(p); }


    FixedBlockAllocator(const FixedBlockAllocator&)            = delete;
    FixedBlockAllocator& operator=(const FixedBlockAllocator&) = delete;


protected:
    SmartShelf<struct _FBA_metadata> underlying_shelf;
};


/***************************************************************************/
/*                                                                         */
/* Implementation-only details follow                                      */
/*                                                                         */
/***************************************************************************/

inline void* FixedBlockAllocator::from_Offset(Offset p) {
    return underlying_shelf.from_Offset(p);
}

inline Offset FixedBlockAllocator::to_Offset(void* p) {
    return underlying_shelf.to_Offset(p);
}

}

#endif
