#ifndef _NVMM_SHELF_REGION_H_
#define _NVMM_SHELF_REGION_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "nvmm/error_code.h"
#include "shelf_mgmt/shelf_file.h"

namespace nvmm {

class ShelfRegion
{
public:
    ShelfRegion() = delete;
    ShelfRegion(std::string pathname);
    ~ShelfRegion();

    ErrorCode Create(size_t size);
    ErrorCode Destroy();
    ErrorCode Verify();    

    bool IsOpen() const
    {
        return is_open_;
    }
    
    ErrorCode Open(int flags);
    ErrorCode Close();
    size_t Size();
    
    ErrorCode Map(void *addr_hint, size_t length, int prot, int flags, loff_t offset, void **mapped_addr);
    ErrorCode Unmap(void *mapped_addr, size_t length);
    
private:
    bool is_open_;    
    ShelfFile shelf_;
};

} // namespace nvmm
#endif
