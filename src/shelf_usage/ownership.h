#ifndef _NVMM_OWNERSHIP_H_
#define _NVMM_OWNERSHIP_H_

#include <stddef.h>
#include <stdint.h>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"
#include "common/process_id.h"

namespace nvmm{

/*
  ownership layout:
  - header
    - uint64_t magic_num;
    - size_t size;
    - size_t item_count;
  - item[list_count];
 */    
class Ownership
{
public:
    using RecoverFn = std::function<ErrorCode (ShelfIndex)>;    
    
    Ownership() = delete;
    // addr must be aligned to cacheline size
    // avail_size is the available space in the shelf starting from addr        
    Ownership(void *addr, size_t avail_size);
    ~Ownership();

    // after calling Create/Destroy, Size() will return the actual size of memory consumed by this
    // data structure
    ErrorCode Create(size_t item_count);
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

    bool AcquireItem(size_t item_idx);
    bool ReleaseItem(size_t item_idx);
    bool CheckItem(size_t item_idx);
    void CheckAndRevokeItem(size_t item_idx);
    void CheckAndRevokeItem(size_t item_idx, RecoverFn func);
    size_t Count() const
    {
        return item_count_;
    }
    
private:
    static uint64_t const kMagicNum = 696377447; // ownership
    
    bool is_open_;
    void *addr_; // the address this data structure is mapped to
    size_t size_; // size of the data structure in FAM
    ProcessID pid_;
    
    size_t item_count_;
    ProcessID *items_; // an array of ProcessIDs (pid + btime)
};

} // namespace nvmm

#endif
