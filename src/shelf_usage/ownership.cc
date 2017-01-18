#include <stddef.h>
#include <stdint.h>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"
#include "common/process_id.h"

#include "nvmm/log.h"
#include "nvmm/nvmm_fam_atomic.h"
#include "nvmm/nvmm_libpmem.h"
#include "common/common.h"

#include "shelf_usage/ownership.h"

namespace nvmm{

struct ownership_header
{
    uint64_t magic_num;
    size_t size; // size of the header and the items
    size_t item_count;
};

Ownership::Ownership(void *addr, size_t avail_size)
    : is_open_{false}, addr_{addr}, size_{avail_size}, pid_{}, item_count_{0}, items_{NULL}
{
    assert(addr != NULL);
    assert((uint64_t)addr%kCacheLineSize == 0);        
}

Ownership::~Ownership()
{
    if(IsOpen() == true)
    {
        (void)Close();
    }    
}
   
ErrorCode Ownership::Create(size_t item_count)
{
    assert(IsOpen() == false);
    assert(item_count != 0);

    char *cur = (char*)addr_;
    size_t cur_size = size_;
    
    // clear header
    size_t header_size = sizeof(ownership_header);
    header_size = round_up(header_size, kCacheLineSize);
    if (cur_size < header_size)
    {
        LOG(error) << "Ownership: insufficient space for header";
        return OWNERSHIP_CREATE_FAILED;
    }
    memset((char*)cur, 0, header_size);        
    pmem_persist(cur, header_size);

    // init items
    cur+=header_size;
    cur_size-=header_size;
    size_t items_size = item_count * sizeof(ProcessID);
    items_size = round_up(items_size, kCacheLineSize);
    if (cur_size < items_size)
    {
        LOG(error) << "Ownership: insufficient space for free stacks";
        return OWNERSHIP_CREATE_FAILED;
    }
    memset((char*)cur, 0, items_size);    
    pmem_persist(cur, items_size);

    // set header
    // set item_count
    ((ownership_header*)addr_)->item_count = item_count;
    // set size of header and the items
    ((ownership_header*)addr_)->size = header_size + items_size;    
    pmem_persist(addr_, header_size);
    
    // finally set magic number
    ((ownership_header*)addr_)->magic_num = kMagicNum;
    pmem_persist(addr_, header_size);

    // update actual size
    size_ = ((ownership_header*)addr_)->size;
    
    return NO_ERROR;
}
    
ErrorCode Ownership::Destroy()
{
    assert(IsOpen() == false);

    if (Verify() == true)
    {
        size_t size = ((ownership_header*)addr_)->size;
        size_ = size;
        memset((char*)addr_, 0, size);    
        pmem_persist(addr_, size);
        return NO_ERROR;
    }
    else
    {
        return OWNERSHIP_DESTROY_FAILED;
    }
}
    
bool Ownership::Verify()
{
    return fam_atomic_u64_read((uint64_t*)addr_) == kMagicNum;
}

ErrorCode Ownership::Open()
{
    assert(IsOpen() == false);

    char *cur = (char*)addr_;

    // verify header
    if(Verify() == false)
    {
        LOG(error) << "Ownership: header->magic_num does not match";
        return OWNERSHIP_OPEN_FAILED;
    }
    if (((ownership_header*)cur)->size > size_)
    {
        LOG(error) << "Ownership: insufficient in this shelf";
        return OWNERSHIP_OPEN_FAILED;
    }

    size_ = ((ownership_header*)cur)->size;    
    item_count_ = ((ownership_header*)cur)->item_count;

    size_t header_size = sizeof(ownership_header);
    header_size = round_up(header_size, kCacheLineSize);
    items_ = (ProcessID*)((char*)cur + header_size);
    
    // everything looks good
    pid_.SetPid(); // initialize our pid
    is_open_ = true;

    return NO_ERROR;
}
    
ErrorCode Ownership::Close()
{
    assert(IsOpen() == true);
    is_open_ = false;
    return NO_ERROR;
}
    
bool Ownership::AcquireItem(size_t item_idx)
{
    assert(IsOpen() == true);
    ProcessID oldval; // 0
    ProcessID result;
    pid_.SetPid();
    fam_atomic_u128_compare_and_store((uint64_t*)&items_[item_idx],
                                      (uint64_t*)&oldval, (uint64_t*)&pid_, (uint64_t*)&result);
    //LOG(fatal)<<"AcquireItem " << item_idx << " by process "  << pid_;
    return result==oldval;
}

bool Ownership::ReleaseItem(size_t item_idx)
{
    assert(IsOpen() == true);
    ProcessID newval; // 0
    ProcessID result;
    fam_atomic_u128_compare_and_store((uint64_t*)&items_[item_idx],
                                      (uint64_t*)&pid_, (uint64_t*)&newval, (uint64_t*)&result);
    //LOG(fatal)<<"ReleaseItem " << item_idx << " by process "  << result;
    return result==pid_;
}

bool Ownership::CheckItem(size_t item_idx)
{
    assert(IsOpen() == true);
    ProcessID result;
    fam_atomic_u128_read((uint64_t*)&items_[item_idx], (uint64_t*)&result);
    return result.IsValid();
}

void Ownership::CheckAndRevokeItem(size_t item_idx)
{
    assert(IsOpen() == true);
    ProcessID oldval;
    fam_atomic_u128_read((uint64_t*)&items_[item_idx], (uint64_t*)&oldval);
    if (oldval.IsValid()==true)
        LOG(trace) << "Ownership: checking ownership of heap " << item_idx << ": pid " << oldval;
    if (oldval.IsValid() == true && oldval.IsAlive() == false)
    {
        ProcessID newval; // 0
        ProcessID result;
        // clear the ProcessID
        fam_atomic_u128_compare_and_store((uint64_t*)&items_[item_idx],
                                          (uint64_t*)&oldval, (uint64_t*)&newval, (uint64_t*)&result);
        if (result == oldval)
        {
            LOG(fatal) << "Ownership: successfully revoking ownership of process " << oldval
                       << " of heap " << item_idx;
        }
        else
        {
            LOG(fatal) << "Ownership: failed to revoke ownership of process " << oldval
                       << " of heap " << item_idx;                
        }
    }
}

void Ownership::CheckAndRevokeItem(size_t item_idx, RecoverFn recover_func)
{
    assert(IsOpen() == true);
    ProcessID oldval;
    fam_atomic_u128_read((uint64_t*)&items_[item_idx], (uint64_t*)&oldval);
    if (oldval.IsValid()==true)
        LOG(trace) << "Ownership: checking ownership of heap " << item_idx << ": pid " << oldval;
    if (oldval.IsValid() == true && oldval.IsAlive() == false)
    {
        ProcessID result;
        pid_.SetPid();
        // acquire the heap and do recovery
        fam_atomic_u128_compare_and_store((uint64_t*)&items_[item_idx],
                                          (uint64_t*)&oldval, (uint64_t*)&pid_, (uint64_t*)&result);
        if (result == oldval)
        {
            LOG(fatal) << "Ownership: start recovery for process " << oldval
                       << " of heap " << item_idx;

            // do recovery
            ErrorCode ret = recover_func((ShelfIndex)item_idx);
            if (ret!=NO_ERROR)
            {
                LOG(fatal) << "Ownership: failed to recover heap " << item_idx;
                // write the oldval back; let others try to recover the heap
                fam_atomic_u128_write((uint64_t*)&items_[item_idx], (uint64_t*)&oldval);
                return;
            }
            else
            {
                LOG(fatal) << "Ownership: heap " << item_idx << " recovered";
                // release our ownership
                ProcessID newval;
                fam_atomic_u128_write((uint64_t*)&items_[item_idx], (uint64_t*)&newval);
                return;
            }
        }
        else
        {
            LOG(fatal) << "Ownership: someone else is doing recovery for process " << oldval
                       << " of heap " << item_idx;                
        }
    }
}
    
} // namespace nvmm
