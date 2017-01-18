#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED
#include <gtest/gtest.h>

#include "nvmm/nvmm_fam_atomic.h"
#include "nvmm/error_code.h"
#include "nvmm/memory_manager.h"
#include "nvmm/shelf_id.h"

#include "test_common/test.h"

#include "shelf_mgmt/shelf_file.h"
#include "shelf_mgmt/shelf_name.h"

#include "shelf_usage/freelists.h"

using namespace nvmm;

static size_t const kShelfSize = 128*1024*1024LLU; // 128 MB
static size_t const kListCount = 16;
static int const ptr_count = 10;

TEST(FreeLists, Test)
{
    ShelfName shelf_name;
    ShelfId const shelf_id(1);
    PoolId const pool_id = shelf_id.GetPoolId();
    std::string path = shelf_name.Path(shelf_id);
    ShelfFile shelf(path);
    EXPECT_EQ(NO_ERROR, shelf.Create(S_IRUSR|S_IWUSR, kShelfSize));

    void* address = NULL;
    
    // mmap
    EXPECT_EQ(NO_ERROR, shelf.Open(O_RDWR));
    EXPECT_EQ(NO_ERROR, shelf.Map(NULL, kShelfSize, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address));

    size_t length = kShelfSize/2;
    loff_t offset = kShelfSize/2;
    FreeLists freelists((char*)address+offset, length);
    
    // create freelists
    EXPECT_EQ(NO_ERROR, freelists.Create(kListCount));

    // open the freelists structure
    EXPECT_EQ(NO_ERROR, freelists.Open());

    GlobalPtr ptr;
    for (size_t i = 0; i< freelists.Count(); i++)
    {
        for (int j = 0; j < ptr_count; j++)
        {            
            EXPECT_EQ(FREELISTS_EMPTY, freelists.GetPointer((ShelfIndex)i, ptr));
        }
    }       

    for (size_t i = 0; i< freelists.Count(); i++)
    {
        for (int j = 0; j < ptr_count; j++)
        {
            EXPECT_EQ(NO_ERROR, freelists.PutPointer((ShelfIndex)i,
                                                     GlobalPtr(ShelfId(pool_id, (ShelfIndex)i), (Offset)j)));
        }
    }       

    for (size_t i = 0; i< freelists.Count(); i++)
    {
        for (int j = ptr_count-1; j >= 0; j--)
        {
            EXPECT_EQ(NO_ERROR, freelists.GetPointer((ShelfIndex)i, ptr));
            EXPECT_EQ(GlobalPtr(ShelfId(pool_id, (ShelfIndex)i), (Offset)j), ptr);
        }
    }       

    EXPECT_EQ(NO_ERROR, freelists.Close());

    // destroy freelists
    EXPECT_EQ(NO_ERROR, freelists.Destroy());        
    
    // unmap
    EXPECT_EQ(NO_ERROR, shelf.Unmap(address, kShelfSize));
    EXPECT_EQ(NO_ERROR, shelf.Close());        
    
    EXPECT_EQ(NO_ERROR, shelf.Destroy());    
}

int main(int argc, char** argv)
{
    InitTest();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

