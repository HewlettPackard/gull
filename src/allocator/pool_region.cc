#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <string>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"
#include "nvmm/region.h"

#include "nvmm/log.h"

#include "shelf_mgmt/pool.h"

#include "shelf_mgmt/shelf_file.h"

#include "shelf_usage/shelf_region.h"

#include "allocator/pool_region.h"

namespace nvmm {
    
PoolRegion::PoolRegion(PoolId pool_id)
    : pool_id_{pool_id}, pool_{pool_id},
      size_{0}, region_file_{NULL}, is_open_{false}
{
}
    
PoolRegion::~PoolRegion()
{
    if(IsOpen() == true)
    {
        (void)Close();
    }
}

ErrorCode PoolRegion::Create(size_t size)
{
    TRACE();
    assert(IsOpen() == false);
    if (size == 0)
    {
        LOG(error) << "Invalid size: " << size;
        return INVALID_ARGUMENTS;
    }
    if (pool_.Exist() == true)
    {
        return POOL_FOUND;
    }
    else
    {
        ErrorCode ret = NO_ERROR;

        // create an empty pool
        ret = pool_.Create(size);
        if (ret != NO_ERROR)
        {
            if (ret == POOL_FOUND)
                return POOL_FOUND;
            else
                return REGION_CREATE_FAILED;
        }

        // add one shelf
        ret = pool_.Open(false);
        if (ret != NO_ERROR)
        {
            return REGION_CREATE_FAILED;
        }
        ShelfIndex shelf_idx=kShelfIdx;
        ret = pool_.AddShelf(shelf_idx,
                             [](ShelfFile *shelf, size_t size)
                             {
                                 ShelfRegion shelf_region(shelf->GetPath());
                                 return shelf_region.Create(size);
                             },
                             false
                            );
        if (ret != NO_ERROR)
        {
            (void)pool_.Close(false);
            return REGION_CREATE_FAILED;
        }
        assert(shelf_idx == kShelfIdx);
        
        ret = pool_.Close(false);
        if (ret != NO_ERROR)
        {
            return REGION_CREATE_FAILED;
        }

        return ret;
    }
}

ErrorCode PoolRegion::Destroy()
{
    TRACE();
    assert(IsOpen() == false);    
    if (pool_.Exist() == false)
    {
        return POOL_NOT_FOUND;
    }
    else
    {
        ErrorCode ret = NO_ERROR;

        // remove the shelf
        ret = pool_.Open(false);
        if (ret != NO_ERROR)
        {
            return REGION_DESTROY_FAILED;
        }
        // ret = pool_.Recover();
        // if(ret != NO_ERROR)
        // {
        //     LOG(fatal) << "Destroy: Found inconsistency in Region " << (uint64_t)pool_id_;
        // }        
        for (ShelfIndex shelf_idx = 0; shelf_idx < pool_.Size(); shelf_idx++)
        {
            if (pool_.CheckShelf(shelf_idx) == true)
            {
                ret = pool_.RemoveShelf(shelf_idx);
                if (ret != NO_ERROR)
                {
                    (void)pool_.Close(false);
                    return REGION_DESTROY_FAILED;
                }
            }
        }
        ret = pool_.Close(false);
        if (ret != NO_ERROR)
        {
            return REGION_DESTROY_FAILED;
        }

        // destroy the pool
        ret = pool_.Destroy();
        if (ret != NO_ERROR)
        {
            return REGION_DESTROY_FAILED;
        }
        return ret;
    }
}

bool PoolRegion::Exist()
{
    return pool_.Exist();
}

ErrorCode PoolRegion::Open(int flags)
{
    TRACE();
    LOG(trace) << "Open Region " << (uint64_t)pool_id_;
    assert(IsOpen() == false);
    ErrorCode ret = NO_ERROR;

    // open the pool
    ret = pool_.Open(false);
    if (ret != NO_ERROR)
    {
        return REGION_OPEN_FAILED;
    }

    // // perform recovery
    // ret = pool_.Recover();
    // if(ret != NO_ERROR)
    // {
    //     LOG(fatal) << "Open: Found inconsistency in Region " << (uint64_t)pool_id_;
    // }

    // get the shelf
    std::string path;
    ret = pool_.GetShelfPath(kShelfIdx, path);
    if (ret != NO_ERROR)
    {
        return REGION_OPEN_FAILED;
    }

    // open ShelfRegion
    region_file_ = new ShelfRegion(path);
    assert (region_file_ != NULL);
    ret = region_file_->Open(flags);
    if (ret != NO_ERROR)
    {
        return REGION_OPEN_FAILED;
    }

    size_ = region_file_->Size();

    is_open_ = true;

    assert(ret == NO_ERROR);
    return ret;
}

ErrorCode PoolRegion::Close()
{
    TRACE();
    LOG(trace) << "Close Region " << pool_id_;
    assert(IsOpen() == true);
    ErrorCode ret = NO_ERROR;
    
    // close the region_file
    ret = region_file_->Close();
    if (ret != NO_ERROR)
    {
        return REGION_CLOSE_FAILED;
    }
    delete region_file_;
    region_file_ = NULL;
    
    // // perform recovery
    // ret = pool_.Recover();
    // if(ret != NO_ERROR)
    // {
    //     LOG(fatal) << "Close: Found inconsistency in Region " << (uint64_t)pool_id_;
    // }
    
    // close the pool
    ret = pool_.Close(false);
    if (ret != NO_ERROR)
    {
        return REGION_CLOSE_FAILED;
    }

    size_ = 0;
    is_open_ = false;

    assert(ret == NO_ERROR);    
    return ret;
}

ErrorCode PoolRegion::Map(void *addr_hint, size_t length, int prot, int flags, loff_t offset, void **mapped_addr)
{
    TRACE();
    assert(IsOpen() == true);
    ErrorCode ret = NO_ERROR;
    ret = region_file_->Map(addr_hint, length, prot, flags, offset, mapped_addr);
    if (ret != NO_ERROR)
    {
        ret = REGION_MAP_FAILED;
    }
    return ret;
}

ErrorCode PoolRegion::Unmap(void *mapped_addr, size_t length)
{
    TRACE();
    assert(IsOpen() == true);
    ErrorCode ret = NO_ERROR;
    ret = region_file_->Unmap(mapped_addr, length);
    if (ret != NO_ERROR)
    {
        ret = REGION_UNMAP_FAILED;
    }
    return ret;
}

} // namespace nvmm
