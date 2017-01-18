#ifndef _NVMM_FREE_LISTS_H_
#define _NVMM_FREE_LISTS_H_

#include <stddef.h>
#include <stdint.h>

#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"
#include "nvmm/shelf_id.h"

namespace nvmm{

class Stack;
class FixedBlockAllocator;
    
/*
  freelists layout:
  - header
    - uint64_t magic_num;
    - size_t size;
    - size_t list_count;
  - Stack freelists[list_count];
  - fba
 */    
class FreeLists
{
public:
    FreeLists() = delete;   
    // addr must be aligned to cacheline size
    // avail_size is the available space in the shelf starting from addr    
    FreeLists(void *addr, size_t avail_size); 
    ~FreeLists();

    // after calling Create/Destroy, Size() will return the actual size of memory consumed by this
    // data structure    
    ErrorCode Create(size_t list_count);
    ErrorCode Destroy();
    bool Verify();
    bool IsOpen() const
    {
        return is_open_;
    }
    size_t Size() const
    {
        return size_;
    }
    
    ErrorCode Open();
    ErrorCode Close();

    // caller must make sure ptr is valid (i.e., same pool id, valid shelf index)
    ErrorCode PutPointer(ShelfIndex shelf_idx, GlobalPtr ptr);
    ErrorCode GetPointer(ShelfIndex shelf_idx, GlobalPtr &ptr);
    size_t Count() const
    {
        return list_count_;
    }
        
private:
    static uint64_t const kMagicNum = 373354787; // freelists
    
    bool is_open_;
    void *addr_; // the address this data structure is mapped to
    size_t size_; // size of the data structure in FAM

    size_t list_count_;
    Stack *freelists_;
    FixedBlockAllocator *fba_;
};

} // namespace nvmm

#endif
