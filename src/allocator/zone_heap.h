#ifndef _NVMM_ZONE_HEAP_H_
#define _NVMM_ZONE_HEAP_H_

#include <stddef.h>
#include <stdint.h>

#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"
#include "nvmm/shelf_id.h"
#include "nvmm/heap.h"

#include "shelf_mgmt/pool.h"

namespace nvmm{

class ShelfHeap;
    
// single-shelf heap
class ZoneHeap : public Heap
{
public:
    ZoneHeap() = delete;
    ZoneHeap(PoolId pool_id);
    ~ZoneHeap();
    
    ErrorCode Create(size_t shelf_size);
    ErrorCode Destroy();
    bool Exist();

    ErrorCode Open();
    ErrorCode Close();
    size_t Size();
    bool IsOpen()
    {
        return is_open_;
    }
    
    GlobalPtr Alloc (size_t size);
    void Free (GlobalPtr global_ptr);

    void *GlobalToLocal(GlobalPtr global_ptr);    
    // TODO: not implemented yet
    GlobalPtr LocalToGlobal(void *addr);

private:
    static int const kShelfIdx = 0; // this is the only shelf in the pool
    
    PoolId pool_id_;
    Pool pool_;
    size_t size_;
    ShelfHeap *rmb_;
    bool is_open_;
};
} // namespace nvmm

#endif
