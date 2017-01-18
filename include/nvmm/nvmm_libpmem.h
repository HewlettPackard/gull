#ifndef _NVMM_LIBPMEM_H_
#define _NVMM_LIBPMEM_H_

#include <libpmem.h>
#include <assert.h>

namespace nvmm {

#ifdef LFS
inline void pmem_persist(void *addr, size_t len)
{
    int ret = pmem_msync(addr, len);
    if (ret==-1)
    {
        assert(ret==0);
        exit(1);
    }
    return;
}
#endif // LFS

} // namespace nvmm

#endif

