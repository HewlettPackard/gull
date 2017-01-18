#include "nvmm/error_code.h"
#include "nvmm/log.h"
#include "nvmm/memory_manager.h"

#include "nvmm/heap.h"

using namespace nvmm;

int main(int argc, char **argv)
{
    init_log(off); // turn off logging

    MemoryManager *mm = MemoryManager::GetInstance();

    // create a new 128MB NVM region with pool id 2
    PoolId pool_id = 2;
    size_t size = 128*1024*1024; // 128MB
    ErrorCode ret = mm->CreateHeap(pool_id, size);
    assert(ret == NO_ERROR);

    // acquire the heap
    Heap *heap = NULL;
    ret = mm->FindHeap(pool_id, &heap);
    assert(ret == NO_ERROR);

    // open the heap
    ret =  heap->Open();
    assert(ret == NO_ERROR);

    // use the heap
    GlobalPtr ptr = heap->Alloc(sizeof(int));  // Alloc returns a GlobalPtr consisting of a shelf ID
                                               // and offset
    assert(ptr.IsValid() == true);

    int *int_ptr = (int*)mm->GlobalToLocal(ptr);  // convert the GlobalPtr into a local pointer

    *int_ptr = 123;
    assert(*int_ptr == 123);

    heap->Free(ptr);

    // close the heap
    ret = heap->Close();
    assert(ret == NO_ERROR);

    // release the heap
    delete heap;

    // delete the heap
    ret = mm->DestroyHeap(pool_id);
    assert(ret == NO_ERROR);    
}

