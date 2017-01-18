#ifndef _NVMM_CONVMMON_H_
#define _NVMM_CONVMMON_H_

namespace nvmm {

/*    
 * System-specific parameters
 */
static int const kCacheLineSize = 64;
static int const kVirtualPageSize = 64*1024; 

/*
 * Round non-negative x up to the nearest multiple of positive
 * multiple
 */
static inline size_t round_up(size_t x, size_t multiple) {
    return ((x+multiple-1)/multiple)*multiple;
}

/*
 * Round non-negative x down to the nearest multiple of positive
 * multiple
 */
static inline size_t round_down(size_t x, size_t multiple) {
    return x/multiple*multiple;
}
    
} // namespace nvmm
    
#endif
