#ifndef _NVMM_SHELF_MANAGER_H_
#define _NVMM_SHELF_MANAGER_H_

#include <mutex>
#include <map>
#include <unordered_map>
#include <stddef.h>

#include "nvmm/shelf_id.h"

namespace nvmm{

// the ShelfManager keeps two mappings:
// - shelf ID => base ptr and length
// - base ptr => shelf ID and length    
// NOTE: to register for a shelf, the shelf must be mapped entirely (no partial mmap)
// TODO: to unregister a shelf safely, we have to make sure the shelf is not being accessed
// use reference counts???
// TODO: should also store file handle    
class ShelfManager
{
public:
    /*
      called by ShelfFile
    */
    // register a shelf's ID, base ptr, and length
    static void *RegisterShelf(ShelfId shelf_id, void *base, size_t length);
    // unregister a shelf's ID, base ptr, and length
    // for now we only unregister a shelf when the shelf is being destroyed (deleted)
    static void *UnregisterShelf(ShelfId shelf_id);
    // check if a shelf is registered or not and return its base if it is registered
    static void *LookupShelf(ShelfId shelf_id);

    /*
      called by MemoryManager
    */
    // given a shelf's ID, return the base of the shelf
    static void *FindBase(ShelfId shelf_id);
    // given a shelf's pathname and shelf ID, register it, map it, and return the base of the shelf
    static void *FindBase(std::string path, ShelfId shelf_id);
    // given a local pointer backed by a shelf, return the shelf's ID and its base pointer
    static ShelfId FindShelf(void *ptr, void *&base);
    // unmap everything, clear both mappings
    static void Reset();

    
    static void Lock();
    static void Unlock();    

private:
    static std::mutex map_mutex_; // guard concurrent access to map_ and rmap_ (more specifically,
                                  // mapping/unmapping/finding shelves)
    // shelf ID => base ptr and length
    static std::unordered_map<ShelfId, std::pair<void *, size_t>, ShelfId::Hash, ShelfId::Equal> map_;
    // base ptr => shelf ID and length
    static std::map<void*, std::pair<ShelfId, size_t>> reverse_map_;
    
};

} // namespace nvmm
    
#endif
