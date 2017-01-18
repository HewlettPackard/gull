/*
  modified from the stack implementation from Mark
 */

#ifndef _NVMM_STACK_H_
#define _NVMM_STACK_H_

#include "nvmm/global_ptr.h"
#include "shelf_usage/smart_shelf.h"


namespace nvmm {
    
/**
 ** A very simple lock-free stack of blocks
 ** 
 ** Must be allocated in FAM.
 ** 
 ** The blocks pushed on the stack must be cache line aligned, at
 ** least a cache line long, and not accessed by anyone else while on
 ** the stack.  They must also all belong to the same Shelf, which must
 ** be passed to pop and push.
 **/
struct Stack {
    // we access the following two fields atomically via 128-bit CAS:
    Offset head __attribute__ ((aligned (16)));
    uint64_t  aba_counter;  // incremented each time head is written

    // returns 0 if stack is empty
    Offset pop (SmartShelf_& shelf);
    void     push(SmartShelf_& shelf, Offset block);

    // returns 0 if stack is empty
    Offset pop (void *addr);
    void     push(void *addr, Offset block);
    
private:
    Stack(const Stack&);              // disable copying
    Stack& operator=(const Stack&);   // disable assignment
};


}

#endif
