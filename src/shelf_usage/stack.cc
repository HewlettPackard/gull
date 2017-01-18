/*
  modified from the stack implementation from Mark
 */

#include <assert.h>

#include "nvmm/global_ptr.h"
#include "shelf_usage/smart_shelf.h"

#include "nvmm/nvmm_fam_atomic.h"
#include "nvmm/nvmm_libpmem.h"

#include "shelf_usage/stack.h"

namespace nvmm {

void Stack::push(SmartShelf_& shelf, Offset block) {
    assert(block != 0);

    uint64_t* b = (uint64_t*) shelf[block];

    uint64_t old[2], store[2], result[2];
    // would non-atomic reads be faster here?
    fam_atomic_u128_read(&head, old);
    for (;;) {
        // would an atomic write be faster here?
        fam_atomic_u64_write(b, old[0]);
        //*b = old[0];
        //pmem_persist(b, sizeof(uint64_t));

        store[0] = block;
        store[1] = old[1] + 1;
        fam_atomic_u128_compare_and_store(&head, old, store, result);
        if (result[0]==old[0] && result[1]==old[1])
            return;

        old[0] = result[0];
        old[1] = result[1];
    }
}

Offset Stack::pop(SmartShelf_& shelf) {
    uint64_t old[2], store[2], result[2];
    // would non-atomic reads be faster here?
    fam_atomic_u128_read(&head, old);
    for (;;) {
        Offset block = old[0];
        if (block == 0)
            break;
        uint64_t* b = (uint64_t*) shelf[block];
        // would an atomic read be faster here?
        Offset next = fam_atomic_u64_read(b);
        //Offset next = *b;

        store[0] = next;
        store[1] = old[1] + 1;
        fam_atomic_u128_compare_and_store(&head, old, store, result);
        if (result[0]==old[0] && result[1]==old[1])
            return block;

        old[0] = result[0];
        old[1] = result[1];
    }

    return 0;
}

void Stack::push(void *addr, Offset block) {
    assert(block != 0);

    uint64_t* b = (uint64_t*)(
                            (char*)addr + block
                            );

    uint64_t old[2], store[2], result[2];
    // would non-atomic reads be faster here?
    fam_atomic_u128_read(&head, old);
    for (;;) {
        // would an atomic write be faster here?
        fam_atomic_u64_write(b, old[0]);
        //*b = old[0];
        //pmem_persist(b, sizeof(uint64_t));

        store[0] = block;
        store[1] = old[1] + 1;
        fam_atomic_u128_compare_and_store(&head, old, store, result);
        if (result[0]==old[0] && result[1]==old[1])
            return;

        old[0] = result[0];
        old[1] = result[1];
    }
}

Offset Stack::pop(void *addr) {
    uint64_t old[2], store[2], result[2];
    // would non-atomic reads be faster here?
    fam_atomic_u128_read(&head, old);
    for (;;) {
        Offset block = old[0];
        if (block == 0)
            break;
        uint64_t* b = (uint64_t*)(
                                (char*)addr + block
                                );
        // would an atomic read be faster here?
        Offset next = fam_atomic_u64_read(b);
        //Offset next = *b;

        store[0] = next;
        store[1] = old[1] + 1;
        fam_atomic_u128_compare_and_store(&head, old, store, result);
        if (result[0]==old[0] && result[1]==old[1])
            return block;

        old[0] = result[0];
        old[1] = result[1];
    }

    return 0;
}


}
