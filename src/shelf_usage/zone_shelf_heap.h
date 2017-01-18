#ifndef _NVMM_ZONE_SHELF_HEAP_H_
#define _NVMM_ZONE_SHELF_HEAP_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"
#include "shelf_mgmt/shelf_file.h"

namespace nvmm {

class Zone;
    
// based on zone::GlobalHeap
class ShelfHeap
{
public:
    ShelfHeap() = delete;
    // the shelf file must already exist
    ShelfHeap(std::string pathname);    
    ShelfHeap(std::string pathname, ShelfId shelf_id);    
    ~ShelfHeap();

    ErrorCode Create(size_t size);        
    ErrorCode Destroy();    
    ErrorCode Verify();
    ErrorCode Recover();
    
    bool IsOpen() const
    {
        return is_open_;
    }
    
    ErrorCode Open();    
    ErrorCode Close();
    size_t Size();

    Offset Alloc(size_t size);
    void Free(Offset offset);

    bool IsValidOffset(Offset offset);
    // TODO: not implemented
    //bool IsValidPtr(void *addr); 
    
    void *OffsetToPtr(Offset offset) const;

    // TODO: not implemented    
    Offset PtrToOffset(void *addr);
        
private:
    ErrorCode OpenMapShelf(bool use_shelf_manager=false);
    ErrorCode UnmapCloseShelf(bool use_shelf_manager=false, bool unregister=false);  

    bool is_open_;
    ShelfFile shelf_;
    void *addr_;
    
    Zone *zone_;
};

} // namespace nvmm

#endif
