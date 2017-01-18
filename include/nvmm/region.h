#ifndef _NVMM_REGION_H_
#define _NVMM_REGION_H_

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"

#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED

namespace nvmm{

class Region
{
public:    
    virtual ~Region(){};

    // flags is the same as flags in open()
    // O_RDONLY, O_WRONLY, or O_RDWR
    virtual ErrorCode Open(int flags) = 0;
    virtual ErrorCode Close() = 0;
    virtual bool IsOpen() = 0;

    virtual size_t Size() = 0;

    // prot is the same as prot in mmap(): PROT_NONE, PROT_READ, or PROT_WRITE
    // flags is also the same as flags in mmap(): MAP_SHARED, or MAP_PRIVATE
    virtual ErrorCode Map(void *addr_hint, size_t length, int prot, int flags, loff_t offset,
                          void **mapped_addr) = 0;
    virtual ErrorCode Unmap(void *mapped_addr, size_t length) = 0;
};

} // namespace nvmm
#endif
