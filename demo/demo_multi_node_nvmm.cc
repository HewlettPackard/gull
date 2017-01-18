#include <stdint.h>
#include <stddef.h>
#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/filesystem.hpp>

#include "nvmm/log.h"
#include "nvmm/error_code.h"
#include "nvmm/memory_manager.h"
#include "nvmm/root_shelf.h"

#include "allocator/dist_heap.h"
#include "shelf_usage/freelists.h"
#include "shelf_mgmt/shelf_file.h"

using namespace nvmm;

PoolId const heap_pool_id = 1; // the pool id of the DistHeap
size_t const heap_size = 128*1024*1024LLU;   // 128MB
size_t const comm_size = 128*1024*1024LLU;   // 128MB

int const count = 100; // loop count


std::random_device r;
std::default_random_engine e1(r());
uint32_t rand_uint32(uint32_t min, uint32_t max)
{
    std::uniform_int_distribution<uint32_t> uniform_dist(min, max);
    return uniform_dist(e1);
}

int Init()
{
    ErrorCode ret = NO_ERROR;
    bool fam_registered = false;

    // comm
    ShelfFile *shelf = NULL;
    void* address = NULL;    
    FreeLists *comm = NULL;

    // heap
    MemoryManager *mm = NULL;    
    Heap *heap = NULL;
    
    // check if SHELF_BASE_DIR exists
    std::cout << "Init: Checking if lfs exists..." << std::endl;
    boost::filesystem::path shelf_base_path = boost::filesystem::path(SHELF_BASE_DIR);
    if (boost::filesystem::exists(shelf_base_path) == false)
    {
        std::cout << "Init: LFS does not exist " << SHELF_BASE_DIR << std::endl;
        exit(1);
    }

    // create a root shelf for MemoryManager if it does not exist
    std::cout << "Init: Creating the root shelf if it does not exist..." << std::endl;
    std::string root_shelf_file = std::string(SHELF_BASE_DIR) + "/" + SHELF_USER + "_RePO_ROOT";
    RootShelf root_shelf(root_shelf_file);
    if(root_shelf.Exist() == false)
    {
        if(root_shelf.Create()!=NO_ERROR)
        {
            std::cout << "Init: Failed to create the root shelf file " << root_shelf_file << std::endl;
            exit(1);
        }
    }

    // create the global heap if it does not exist
    mm = MemoryManager::GetInstance();
    std::cout << "Init: Creating the distibuted heap if it does not exist..." << std::endl;
    ret = mm->FindHeap(heap_pool_id, &heap);
    if (ret == ID_NOT_FOUND)
    {
        ret = mm->CreateHeap(heap_pool_id, heap_size);
        if (ret!=NO_ERROR)
            exit(1);
    }
        
    // create a shelf for communication among processes
    // use the FreeLists
    std::cout << "Init: Creating the communication shelf..." << std::endl;
    std::string comm_shelf_file = std::string(SHELF_BASE_DIR) + "/" + SHELF_USER + "_RePO_COMM";    
    shelf = new ShelfFile(comm_shelf_file);
    if (shelf->Exist()==true)
    {
        shelf->Destroy();
    }
    size_t list_count = 1;
    ret = shelf->Create(S_IRUSR|S_IWUSR, comm_size);
    if (ret!=NO_ERROR)
        exit(1);
    ret = shelf->Open(O_RDWR);
    if (ret!=NO_ERROR)
        exit(1);
    ret = shelf->Map(NULL, comm_size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address);
    if (ret!=NO_ERROR)
        exit(1);
    fam_registered = true;
        
    comm = new FreeLists(address, comm_size);
    ret = comm->Create(list_count);
    if (ret!=NO_ERROR)
        exit(1);

    if (comm!=NULL)
        delete comm;

    if (fam_registered == true)
    {
        shelf->Unmap(address, comm_size);
        shelf->Close();
    }
        
    if (shelf!=NULL)
        delete shelf;

    if (heap!=NULL)
        delete heap;
    
    if (ret==NO_ERROR)
        return 0;
    else
        return 1;
}

int Cleanup()
{
    // init boost::log
    nvmm::init_log(nvmm::fatal, "mm.log");
    
    // remove previous files in SHELF_BASE_DIR
    std::cout << "Cleanup: Removing all RePO files in " << SHELF_BASE_DIR << std::endl;
    std::string cmd = std::string("exec rm -f ") + SHELF_BASE_DIR + "/" + SHELF_USER + "* > nul";
    //std::cout << cmd << std::endl;
    return system(cmd.c_str());
}

int Run()
{
    ErrorCode ret = NO_ERROR;

    MemoryManager *mm = MemoryManager::GetInstance();
    size_t size = heap_size;
    for (int i=0; i<count; i++)
    {
        PoolId pool_id = (PoolId)rand_uint32(1, Pool::kMaxPoolCount-1); // 0 is reserved
        Region *region = NULL;
        Heap *heap = NULL;
        
        //ShelfIndex shelf_idx = 0;
        switch (rand_uint32(0,5))
        {
        case 0:
            mm->CreateRegion(pool_id, size);
            break;
        case 1:
            mm->DestroyRegion(pool_id);
            break;
        case 2:
            ret = mm->FindRegion(pool_id, &region);
            if (ret == NO_ERROR && region != NULL)
                delete region;
            break;
        case 3:
            mm->CreateHeap(pool_id, size);
            break;
        case 4:
            mm->DestroyHeap(pool_id);
            break;
        case 5:
            ret = mm->FindHeap(pool_id, &heap);
            if (ret == NO_ERROR && heap != NULL)
                delete heap;
            break;
        default:
            assert(0);
            break;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc!=2)
    {
        std::cout << "Usage: ./" << argv[0] << " init/cleanup/run" << std::endl;
        exit(1);
    }

    // init boost::log
    nvmm::init_log(nvmm::fatal, "mm.log");   
    
    std::string cmd(argv[1]);
    if (cmd == "init")
    {
        return Init();
    }
    else if (cmd == "cleanup")
    {
        return Cleanup();
    }
    else if (cmd == "run")
    {
        return Run();
    }           
}

