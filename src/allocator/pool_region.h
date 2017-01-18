#ifndef _NVMM_POOL_REGION_H_
#define _NVMM_POOL_REGION_H_

#include <stddef.h>
#include <stdint.h>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"
#include "nvmm/region.h"

#include "shelf_mgmt/pool.h"

namespace nvmm{

class ShelfRegion;
    
class PoolRegion : public Region
{
public:    
    PoolRegion() = delete;
    PoolRegion(PoolId pool_id);
    ~PoolRegion();    
    ErrorCode Create(size_t size);
    ErrorCode Destroy();
    bool Exist();
    
    ErrorCode Open(int flags);
    ErrorCode Close();
    size_t Size()
    {
        return size_;
    }
    bool IsOpen()
    {
        return is_open_;
    }
    
    ErrorCode Map(void *addr_hint, size_t length, int prot, int flags, loff_t offset, void **mapped_addr);
    ErrorCode Unmap(void *mapped_addr, size_t length);

private:
    static int const kShelfIdx = 0; // this is the only shelf in the pool

    PoolId pool_id_;    
    Pool pool_;
    size_t size_;
    ShelfRegion *region_file_;
    bool is_open_;
};
    
} // namespace nvmm
#endif
