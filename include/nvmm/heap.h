#ifndef _NVMM_HEAP_H_
#define _NVMM_HEAP_H_

#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"

namespace nvmm{

class Heap
{
public:
    virtual ~Heap(){};

    virtual ErrorCode Open() = 0;
    virtual ErrorCode Close() = 0;
    virtual bool IsOpen() = 0;
    
    virtual GlobalPtr Alloc (size_t size) = 0;
    virtual void Free (GlobalPtr global_ptr) = 0;
};
    
} // namespace nvmm
#endif
