#include <stddef.h>
#include <stdint.h>

#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"
#include "nvmm/shelf_id.h"

#include "nvmm/log.h"
#include "nvmm/nvmm_fam_atomic.h"
#include "nvmm/nvmm_libpmem.h"

#include "common/common.h"

#include "shelf_usage/stack.h"
#include "shelf_usage/fixed_block_allocator.h"

#include "shelf_usage/freelists.h"

namespace nvmm{

struct freelists_header
{
    uint64_t magic_num;
    size_t size; // header + stacks + fba
    size_t list_count;
};
    
struct fba_block
{
    Offset internal_ptr; // used by Stack
    uint64_t free_ptr; // global pointer to free
};

FreeLists::FreeLists(void *addr, size_t avail_size)
    : is_open_{false}, addr_{addr}, size_{avail_size}, list_count_{0}, freelists_{NULL}, fba_{NULL}
{
    assert(addr != NULL);
    assert((uint64_t)addr%kCacheLineSize == 0);            
}

FreeLists::~FreeLists()
{
    if(IsOpen() == true)
    {
        (void)Close();
    }
}
    
ErrorCode FreeLists::Create(size_t list_count)
{
    assert(IsOpen() == false);
    assert(list_count != 0);

    char *cur = (char*)addr_;
    size_t cur_size = size_;
    
    // clear header
    size_t header_size = sizeof(freelists_header);
    header_size = round_up(header_size, kCacheLineSize);
    if (cur_size < header_size)
    {
        LOG(error) << "FreeLists: insufficient space for header";
        return FREELISTS_CREATE_FAILED;
    }
    memset((char*)cur, 0, header_size);        

    // init free stacks
    cur+=header_size;
    cur_size-=header_size;
    size_t freelists_size = list_count * sizeof(Stack);
    freelists_size = round_up(freelists_size, kCacheLineSize);
    if (cur_size < freelists_size)
    {
        LOG(error) << "FreeLists: insufficient space (" << cur_size << ") for free stacks ("
                   << freelists_size << ")";
        return FREELISTS_CREATE_FAILED;
    }
    memset((char*)cur, 0, freelists_size);    
    pmem_persist(cur, freelists_size);
    
    // create the fixed block allocator
    cur+=freelists_size;
    cur_size-=freelists_size;
    if (cur_size <= 0)
    {
        LOG(error) << "FreeLists: insufficient space for fba";
        return FREELISTS_CREATE_FAILED;
    }

    // TODO: catch exceptions
    FixedBlockAllocator(cur, sizeof(fba_block), 0, 0, cur_size);
    
    // set header
    // set list_count
    ((freelists_header*)addr_)->list_count = list_count;
    // set size 
    ((freelists_header*)addr_)->size = header_size + freelists_size + cur_size;
    pmem_persist(addr_, header_size);
    
    // finally set magic number
    ((freelists_header*)addr_)->magic_num = kMagicNum;
    pmem_persist(addr_, header_size);

    // update actual size
    size_ = ((freelists_header*)addr_)->size;
    
    return NO_ERROR;
}
    
ErrorCode FreeLists::Destroy()
{
    assert(IsOpen() == false);

    if (Verify() == true)
    {
        size_t size = ((freelists_header*)addr_)->size;
        size_ = size;
        memset((char*)addr_, 0, size);    
        pmem_persist(addr_, size);
        return NO_ERROR;
    }
    else
    {
        return FREELISTS_DESTROY_FAILED;
    }
}

bool FreeLists::Verify()
{
    return fam_atomic_u64_read((uint64_t*)addr_) == kMagicNum;
}
    
ErrorCode FreeLists::Open()
{
    assert(IsOpen() == false);

    char *cur = (char*)addr_;

    // verify header
    if(Verify() == false)
    {
        LOG(error) << "FreeLists: header->magic_num does not match " << fam_atomic_u64_read((uint64_t*)addr_);
        return FREELISTS_OPEN_FAILED;
    }
    if (((freelists_header*)cur)->size > size_)
    {
        LOG(error) << "FreeLists: header->size does not match";
        return FREELISTS_OPEN_FAILED;
    }

    size_ = ((freelists_header*)cur)->size;
    list_count_ = ((freelists_header*)cur)->list_count;

    size_t header_size = sizeof(freelists_header);
    header_size = round_up(header_size, kCacheLineSize);
    freelists_ = (Stack*)((char*)cur + header_size);
    
    // verify fba
    size_t freelists_size = list_count_ * sizeof(Stack);
    freelists_size = round_up(freelists_size, kCacheLineSize);
    size_t total_header_size = header_size + freelists_size;
    cur+=total_header_size;
    // TODO: catch exceptions
    fba_ = new FixedBlockAllocator(cur, sizeof(fba_block), 0, 0, size_-total_header_size);
    assert(fba_ != NULL);
    
    // everything looks good
    is_open_ = true;

    return NO_ERROR;
}
    
ErrorCode FreeLists::Close()
{
    assert(IsOpen() == true);
    is_open_ = false;
    return NO_ERROR;
}
    
ErrorCode FreeLists::PutPointer(ShelfIndex shelf_idx, GlobalPtr ptr)
{
    assert(IsOpen() == true);

    Offset blk = fba_->alloc();
    if (blk != 0)
    {
        fba_block *fba_blk = (fba_block*)fba_->from_Offset(blk);
        fam_atomic_u64_write(&fba_blk->free_ptr, ptr.ToUINT64());
        freelists_[shelf_idx].push(fba_->get_underlying_shelf(), blk);
        //LOG(trace) << "pool " << (uint64_t)pool_id_ << ": ptr " << ptr << " added to list " << (uint64_t)shelf_idx;
    }
    return NO_ERROR;
}

ErrorCode FreeLists::GetPointer(ShelfIndex shelf_idx, GlobalPtr &ptr)
{
    assert(IsOpen() == true);

    Offset blk = freelists_[shelf_idx].pop(fba_->get_underlying_shelf());
    if (blk != 0)
    {
        fba_block *fba_blk = (fba_block*)fba_->from_Offset(blk);
        ptr = GlobalPtr(fam_atomic_u64_read(&fba_blk->free_ptr));
        fba_->free(blk);
        //LOG(trace) << "pool " << (uint64_t)pool_id_ << ": ptr " << ptr << " removed from list " << (uint64_t)shelf_idx;
        return NO_ERROR;
    }
    else
    {
        return FREELISTS_EMPTY;
    }
}


} // namespace nvmm
