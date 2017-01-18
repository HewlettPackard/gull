#ifndef _NVMM_MEMORY_MANAGER_H_
#define _NVMM_MEMORY_MANAGER_H_

#include <memory>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h" // for PoolId
#include "nvmm/global_ptr.h" // for GlobalPtr

#include "nvmm/heap.h"
#include "nvmm/region.h"

namespace nvmm{

// TODO: Current limitation:
// - No type check for an existing pool id (mm does not know if specified pool is a Heap or a Region)
// - No pool-id management/assignment (clients of mm must agree upon a set of pool ids that each
// wants to use, e.g., 1 and 2 for item store, 3 for metadata store, etc) 
// - No multi-process support for pool (heap/region) creation and destroy
class MemoryManager
{
public:
    // there is only one instance of mm in a process
    // return the instance
    static MemoryManager *GetInstance();

    /*
       methods to access a global pointer with its size known
       internally, mm only maps the pointer + size
       currently used by Zero-copy version of Get and Put
    */
    ErrorCode MapPointer(GlobalPtr ptr, size_t size,
                         void *addr_hint, int prot, int flags, void **mapped_addr);
    ErrorCode UnmapPointer(GlobalPtr ptr, void *mapped_addr, size_t size);

    /*
       methods to access a global pointer, without knowning its size
       internally, mm would use ShelfManager to map entire shelf if it is not mapped
       for general use cases (e.g., accessing a pointer allocated from a Heap)
    */
    void *GlobalToLocal(GlobalPtr ptr);
    GlobalPtr LocalToGlobal(void *addr);
    
    /* 
       a heap provides Alloc/Free APIs
    */
    // Create a heap with the given id
    // NOTE: size is the default shelf size; not the total size of the heap
    // Return
    // - NO_ERROR: heap was created
    // - ID_FOUND: the given id is in use
    ErrorCode CreateHeap(PoolId id, size_t shelf_size); 

    // Destroy the heap with the given id
    // Return
    // - NO_ERROR: heap was destroyed
    // - ID_NOT_FOUND: heap of the given id is not found
    // NOTE
    // - caller must make sure no other processes are still accessing this heap
    ErrorCode DestroyHeap(PoolId id);

    // Find the heap by id and return a pointer to the heap object if it exists
    // Caller is responsible for freeing the poitner
    // Return
    // - NO_ERROR: heap is found and returned
    // - ID_NOT_FOUND: heap of the given id is not found
    // NOTE
    // - It will always return a new heap object even if the heap was already created/opened in the
    // same process
    // - The best use pattern is to find and open a heap once in a process
    ErrorCode FindHeap(PoolId id, Heap **heap);

    // Find the heap by id and return a pointer to the heap object if it exists
    // Caller is responsible for freeing the poitner
    // Return NULL if heap of the given id is not found
    Heap *FindHeap(PoolId id);
    

    /*
      a region provides direct memory mapping APIs
    */
    // Create a region with the given id
    // Return
    // - NO_ERROR: region was created
    // - ID_FOUND: the given id is in use
    ErrorCode CreateRegion(PoolId id, size_t size); 

    // Destroy the region with the given id
    // Return
    // - NO_ERROR: region was destroyed
    // - ID_NOT_FOUND: region of the given id is not found
    // NOTE
    // - caller must make sure no other processes are still accessing this heap
    ErrorCode DestroyRegion(PoolId id);

    // Find the region by id and return a pointer to the region object if it exists
    // Caller is responsible for freeing the poitner
    // Return
    // - NO_ERROR: region is found and returned
    // - ID_NOT_FOUND: region of the given id is not found
    // NOTE
    // - It will always return a new region object even if the region was already created/opened in the
    // same process
    // - The best use pattern is to find and open a region once in a process
    ErrorCode FindRegion(PoolId id, Region **region);

    // Find the region by id and return a pointer to the region object if it exists
    // Caller is responsible for freeing the poitner
    // Return NULL if region of the given id is not found
    Region *FindRegion(PoolId id);
    
private:

    MemoryManager();
    ~MemoryManager();
    
    class Impl_;        
    std::unique_ptr<Impl_> pimpl_;
    
};
} // namespace nvmm   
#endif
