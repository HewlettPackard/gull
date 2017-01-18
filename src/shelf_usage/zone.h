#ifndef _NVMM_ZONE_H_
#define _NVMM_ZONE_H_

#include <stddef.h>
#include <stdint.h>

#include "nvmm/global_ptr.h"

namespace nvmm {

class Zone {
public:
    Zone(void *addr, size_t initial_pool_size, size_t min_object_size,
			size_t max_pool_size);

    Zone(void *addr, size_t max_pool_size);

    virtual ~Zone() {}

    // returns 0 if no blocks are currently available
    Offset alloc(size_t size);
    // [unsafe_]free(0) is a no-op
    void     free       (Offset block);
    void     start_merge();

    // TODO
    // requires all writes to block block have already been persisted
    //    void     unsafe_free(Offset block);

    bool IsValidOffset(Offset p);

    // converting between external Offset (with size encoded) and local-address-space pointers
    void*    OffsetToPtr(Offset p);
    // TODO
    //Offset PtrToOffset (void*    p);
    
    Zone(const Zone&)            = delete;
    Zone& operator=(const Zone&) = delete;


private:
    char *shelf_location_ptr; 

    // shortcut for from_Offset; does not work well on Zone*:
    //   need (*fba)[ptr] for that case
    void* operator[](Offset p) { return from_Offset(p); }
    
    // converting to and from local-address-space pointers (Offset is NOT encoded with size)
    void*    from_Offset(Offset p); 
    Offset to_Offset  (void*    p);

    bool grow();
    bool is_grow_in_progress(struct Zone_Header *zoneheader);
    bool is_merge_in_progress(struct Zone_Header *zoneheader);
    void modify_bitmap_bit(struct Zone_Header *zoneheader, uint64_t level, Offset ptr, bool set);
    void set_bitmap_bit(struct Zone_Header *zoneheader, uint64_t level, Offset ptr);
    void reset_bitmap_bit(struct Zone_Header *zoneheader, uint64_t level, Offset ptr);
    bool merge(struct Zone_Header *zoneheader, uint64_t level);
    void grow_crash_recovery();
    void merge_crash_recovery();
    void detect_lost_chunks();
};


/***************************************************************************/
/*                                                                         */
/* Implementation-only details follow                                      */
/*                                                                         */
/***************************************************************************/

inline void* Zone::from_Offset(Offset p) {
    if (p == 0)
        return NULL;
    else
        return (char*)shelf_location_ptr + p;
}

inline Offset Zone::to_Offset(void* p) {
    if (p == NULL)
        return 0;
    else
        return (char*)p - (char*)shelf_location_ptr;
}

}

#endif
