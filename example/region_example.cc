#include "nvmm/error_code.h"
#include "nvmm/log.h"
#include "nvmm/memory_manager.h"

#include "nvmm/heap.h"

using namespace nvmm;

int main(int argc, char **argv)
{
    init_log(off); // turn off logging

    MemoryManager *mm = MemoryManager::GetInstance();

    // create a new 128MB NVM region with pool id 1
    PoolId pool_id = 1;
    size_t size = 128*1024*1024; // 128MB
    ErrorCode ret = mm->CreateRegion(pool_id, size);
    assert(ret == NO_ERROR);

    // acquire the region
    Region *region = NULL;
    ret = mm->FindRegion(pool_id, &region);
    assert(ret == NO_ERROR);

    // open and map the region
    ret = region->Open(O_RDWR);
    assert(ret == NO_ERROR);
    int64_t* address = NULL;
    ret = region->Map(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address);
    assert(ret == NO_ERROR);

    // use the region
    *address = 123;
    assert(*address == 123);

    // unmap and close the region
    ret = region->Unmap(address, size);
    assert(ret == NO_ERROR);
    ret = region->Close();
    assert(ret == NO_ERROR);

    // release the region
    delete region;

    // delete the region
    ret = mm->DestroyRegion(pool_id);
    assert(ret == NO_ERROR);    
}

