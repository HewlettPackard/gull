#include <stddef.h>
#include <stdint.h>
#include <assert.h> // for assert()
#include <string> 

#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED

#include "nvmm/nvmm_fam_atomic.h"
#include "nvmm/nvmm_libpmem.h"

#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"
#include "common/common.h"
#include "shelf_mgmt/shelf_file.h"

#include "nvmm/log.h"

#include "shelf_usage/shelf_heap.h"

namespace nvmm {

ShelfHeap::ShelfHeap(std::string pathname)
    : is_open_{false}, shelf_{pathname}, addr_{NULL}, layout_{NULL}, size_{0}
{
}

ShelfHeap::ShelfHeap(std::string pathname, ShelfId shelf_id)
    : is_open_{false}, shelf_{pathname, shelf_id}, addr_{NULL}, layout_{NULL}, size_{0}
{
}
    
ShelfHeap::~ShelfHeap()
{
    if (IsOpen() == true)
    {
        (void)Close();
    }
}

ErrorCode ShelfHeap::Create(size_t heap_size)
{
    assert(IsOpen() == false);

    assert(shelf_.Exist() == true);
    
    ErrorCode ret = NO_ERROR;    
    
    // allocate memory for the shelf
    // TODO: make sure actual_size is rounded up to N*8GB
    size_t actual_size = heap_size + NvHeapLayout::kMetadataSize;
    ret = shelf_.Truncate(actual_size);
    if (ret != NO_ERROR)
    {
        return ret;
    }
    
    // then, format the shelf
    ret = OpenMapShelf();
    if (ret != NO_ERROR)
    {
        return ret;
    }
    
    // create the bitmap
    NvHeapLayout::Create(addr_, heap_size);
    
    ret= UnmapCloseShelf();
    if (ret != NO_ERROR)
    {
        return ret;
    }        
    
    return ret;
}
        
ErrorCode ShelfHeap::Destroy()
{
    assert(IsOpen() == false);

    ErrorCode ret = NO_ERROR;

    ret = OpenMapShelf(true);
    if (ret != NO_ERROR)
    {
        return ret;
    }
    
    // destroy the bitmap
    //NvHeapLayout::Destroy(addr_);
    
    ret = UnmapCloseShelf(true, true);
    if (ret != NO_ERROR)
    {
        return ret;
    }        

    // free space for the shelf
    return shelf_.Truncate(0);
}

ErrorCode ShelfHeap::Verify()
{
    assert(IsOpen() == false);
    ErrorCode ret = NO_ERROR;
    ret = OpenMapShelf();
    if (ret != NO_ERROR)
    {
        return ret;
    }
    
    if (shelf_.Size() < sizeof(NvHeapLayout))
    {
        ret = SHELF_FILE_INVALID_FORMAT;
        goto out;
    }
        
    // verify the bitmap
    if (NvHeapLayout::Verify(addr_) == false)
    {
        ret = SHELF_FILE_INVALID_FORMAT;
    }

 out:    
    (void) UnmapCloseShelf();
    return ret;
}

ErrorCode ShelfHeap::Recover()
{
    // nothing to recover
    return NO_ERROR;
}
    
ErrorCode ShelfHeap::Open()
{
    assert(IsOpen() == false);

    ErrorCode ret = NO_ERROR;
                
    ret = OpenMapShelf(true);
    if (ret != NO_ERROR)
    {
        return ret;
    }

    if (shelf_.Size() < sizeof(NvHeapLayout))
    {
        (void) UnmapCloseShelf(true, true);
        return SHELF_FILE_INVALID_FORMAT;
    }
    
    if(NvHeapLayout::Verify(addr_) == false)
    {
        (void) UnmapCloseShelf(true, true);
        return SHELF_FILE_INVALID_FORMAT;
    }

    layout_ = (NvHeapLayout*)addr_;
    assert(layout_->Size() > 0);
    size_ = layout_->Size();
    
    is_open_ = true;
    return ret;
}

ErrorCode ShelfHeap::Close()
{
    assert(IsOpen() == true);

    layout_ = NULL;
    size_ = 0;
    
    ErrorCode ret = UnmapCloseShelf(true, false);
    if (ret == NO_ERROR)
    {
        is_open_ = false;
    }
    return ret;
}

size_t ShelfHeap::Size()
{
    assert(IsOpen() == true);
    return layout_->Size();
}
    
Offset ShelfHeap::Alloc(size_t size)
{
    assert(IsOpen() == true);
    Offset offset;
    offset = layout_->Alloc(size);
    LOG(trace) << "ShelfHeap::Alloc " << offset;
    return offset;
}

void ShelfHeap::Free(Offset offset)
{
    assert(IsOpen() == true);
    layout_->Free(offset);
    LOG(trace) << "ShelfHeap::Free " << offset;
}
    
bool ShelfHeap::IsValidOffset(Offset offset)
{
    assert(IsOpen() == true);
    return layout_->IsValid(offset);
}
    
bool ShelfHeap::IsValidPtr(void *addr)
{
    assert(IsOpen() == true);
    if (addr < layout_)
    {
        return false;
    }
    else
    {
        Offset offset = (Offset)((char*)addr - (char*)layout_);
        return IsValidOffset(offset);
    }
}
    
void *ShelfHeap::OffsetToPtr(Offset offset) const
{
    assert(IsOpen() == true);
    assert(layout_->IsValid(offset) == true);
    return (void*)(((char*)layout_) + offset);
}    

Offset ShelfHeap::PtrToOffset(void *addr)
{
    assert(IsOpen() == true);
    assert(addr>layout_);
    Offset offset = (char*)addr - (char*)layout_;
    assert(layout_->IsValid(offset) == true);
    return offset;
}

ErrorCode ShelfHeap::OpenMapShelf(bool use_shelf_manager)
{
    // // see if this shelf is already mapped
    // if (use_shelf_manager == true)
    // {
    //     addr_ = ShelfManager::FindShelf(shelf_.GetPath());
    //     if (addr_ != NULL)
    //     {
    //         return NO_ERROR;
    //     }
    // }
            
    // check if the shelf exists
    if (shelf_.Exist() == false)
    {
        return SHELF_FILE_NOT_FOUND;
    }
    
    ErrorCode ret = NO_ERROR;

    // open the shelf
    ret = shelf_.Open(O_RDWR);
    if (ret != NO_ERROR)
    {
        return ret;
    }

    // mmap the shelf
    assert(addr_ == NULL);
    if (use_shelf_manager == true)
    {
        ret = shelf_.Map(NULL, &addr_);
    }
    else
    {
        size_t size = shelf_.Size();
        if (size != 0)
        {
            ret = shelf_.Map(NULL, size , PROT_READ|PROT_WRITE, MAP_SHARED, 0, &addr_, true);
            if (ret == NO_ERROR)
            {
                assert(addr_ != NULL);
            }
            else
            {
                return ret;
            }
        }
    }

    // if (use_shelf_manager == true)
    // {
    //     void *addr = ShelfManager::RegisterShelf(shelf_.GetPath(), addr_);
    //     if (addr != addr_)
    //     {
    //         // the shelf was already registered
    //         (void)shelf_.Unmap(addr_, size);
    //         addr_ = addr;
    //     }
    // }
    
    return ret;
}
    
ErrorCode ShelfHeap::UnmapCloseShelf(bool use_shelf_manager, bool unregister)
{
    // if (use_shelf_manager == true)
    // {
    //     // TODO: for now, we don't unregister a shelf once it has been registered
    //     (void)shelf_.Close();
    //     addr_ = NULL;
    //     return NO_ERROR;
    // }
    
    // check if the shelf exists
    if (shelf_.Exist() == false)
    {
        return SHELF_FILE_NOT_FOUND;
    }

    ErrorCode ret = NO_ERROR;

    // unmmap the shelf
    assert(addr_ != NULL);
    size_t size = shelf_.Size();
    if (use_shelf_manager == true)
    {
        ret = shelf_.Unmap(addr_, unregister);
    }
    else
    {
        ret = shelf_.Unmap(addr_, size, true);
    }
    if (ret == NO_ERROR)
    {
        addr_ = NULL;
    }
    else
    {
        return ret;
    }
        
    // close the shelf
    ret = shelf_.Close();
    return ret;        
}
    
} // namespace nvmm
