#include <libpmem.h>
#include <stdint.h>
#include <string.h>
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

void *fam_memcpy(void *dest, const void *src, size_t n)
{
  return memcpy(dest, src, n);
}

int fam_memcmp(const void *s1, const void *s2, size_t n)
{
  return memcmp(s1, s2, n);
}

int64_t fam_read_64(const void *addr)
{
  return *((int64_t*) addr);
}

void fam_read_128(const void *addr, int64_t val[2])
{
  val[0] = ((int64_t*) addr)[0];
  val[1] = ((int64_t*) addr)[1];
}

void fam_fence()
{
  return;
}
