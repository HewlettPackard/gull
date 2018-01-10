#include <libpmem.h>
#include <stdint.h>
#include <emmintrin.h> // for _mm_clflush

void fam_invalidate(const void *addr, size_t len) 
{
/* #ifndef PMEM_INVALIDATE_NOOP */
/*   pmem_invalidate(addr, len); */
/* #else */
  uintptr_t uptr;

  uintptr_t FLUSH_ALIGN = (uintptr_t)64;
  for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
       uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN)
      _mm_clflush((char *)uptr);
/* #endif */
}

void fam_persist(const void *addr, size_t len)
{
  pmem_persist(addr, len);
}

void* fam_memset_persist(void *pmemdest, int c, size_t len) 
{
  return pmem_memset_persist(pmemdest, c, len);
}
