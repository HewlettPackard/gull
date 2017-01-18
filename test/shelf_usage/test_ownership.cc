#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED
#include <gtest/gtest.h>
#include "nvmm/nvmm_fam_atomic.h"

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"

#include "test_common/test.h"

#include "shelf_mgmt/shelf_file.h"
#include "shelf_mgmt/shelf_name.h"

#include "shelf_usage/ownership.h"

using namespace nvmm;

static size_t const kShelfSize = 128*1024*1024LLU; // 128 MB
static size_t const kItemCount = 64;

TEST(Ownership, Test)
{
    ShelfName shelf_name;
    ShelfId const shelf_id(1);
    std::string path = shelf_name.Path(shelf_id);
    ShelfFile shelf(path);
    EXPECT_EQ(NO_ERROR, shelf.Create(S_IRUSR|S_IWUSR, kShelfSize));

    void* address = NULL;
    
    // mmap
    EXPECT_EQ(NO_ERROR, shelf.Open(O_RDWR));
    EXPECT_EQ(NO_ERROR, shelf.Map(NULL, kShelfSize, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address));

    size_t length = kShelfSize/2;
    loff_t offset = kShelfSize/2;    
    Ownership ownership((char*)address+offset, length);
    
    // create the ownership data structure
    EXPECT_EQ(NO_ERROR, ownership.Create(kItemCount));

    // open the ownership structure
    EXPECT_EQ(NO_ERROR, ownership.Open());
    
    for (size_t i = 0; i< ownership.Count(); i++)
    {
        EXPECT_FALSE(ownership.CheckItem(i));
        EXPECT_FALSE(ownership.ReleaseItem(i));
    }       

    for (size_t i = 0; i< ownership.Count(); i++)
    {
        EXPECT_FALSE(ownership.CheckItem(i));
        EXPECT_TRUE(ownership.AcquireItem(i));
        EXPECT_TRUE(ownership.CheckItem(i));
    }       

    for (size_t i = 0; i< ownership.Count(); i++)
    {
        EXPECT_TRUE(ownership.CheckItem(i));
        EXPECT_FALSE(ownership.AcquireItem(i));
        EXPECT_TRUE(ownership.CheckItem(i));
    }       
    
    for (size_t i = 0; i< ownership.Count(); i++)
    {
        EXPECT_TRUE(ownership.ReleaseItem(i));
        EXPECT_FALSE(ownership.CheckItem(i));
    }       

    EXPECT_EQ(NO_ERROR, ownership.Close());

    // destroy ownership
    EXPECT_EQ(NO_ERROR, ownership.Destroy());    
    
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

