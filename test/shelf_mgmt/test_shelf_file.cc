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

using namespace nvmm;

static size_t const kShelfSize = 128*1024*1024LLU; // 128 MB

TEST(ShelfFile, CreateDestroyTruncateExistSize)
{
    ShelfName shelf_name;
    ShelfId const shelf_id(1);
    std::string path = shelf_name.Path(shelf_id);
    ShelfFile shelf(path);
    EXPECT_FALSE(shelf.Exist());
    EXPECT_EQ(NO_ERROR, shelf.Create(S_IRUSR|S_IWUSR));
    EXPECT_TRUE(shelf.Exist());
    EXPECT_EQ((size_t)0, shelf.Size());
    EXPECT_EQ(NO_ERROR, shelf.Truncate( kShelfSize));
    EXPECT_EQ((size_t)kShelfSize, shelf.Size());   
    EXPECT_EQ(NO_ERROR, shelf.Destroy());
    EXPECT_EQ(NO_ERROR, shelf.Create(S_IRUSR|S_IWUSR, kShelfSize));
    EXPECT_TRUE(shelf.Exist());
    EXPECT_EQ(kShelfSize, shelf.Size());
    EXPECT_EQ(NO_ERROR, shelf.Destroy());    
}

TEST(ShelfFile, OpenCloseTruncateExistSize)
{
    ShelfName shelf_name;
    ShelfId const shelf_id(1);
    std::string path = shelf_name.Path(shelf_id);
    ShelfFile shelf(path);
    EXPECT_EQ(NO_ERROR, shelf.Create(S_IRUSR|S_IWUSR));

    EXPECT_EQ(NO_ERROR, shelf.Open(O_RDWR));
    EXPECT_TRUE(shelf.Exist());
    EXPECT_EQ((size_t)0, shelf.Size());

    EXPECT_EQ(NO_ERROR, shelf.Truncate(kShelfSize));
    EXPECT_EQ((size_t)kShelfSize, shelf.Size());

    EXPECT_EQ(NO_ERROR, shelf.Close());
    EXPECT_EQ(NO_ERROR, shelf.Destroy());    
}


TEST(ShelfFile, MapUnmap)
{
    ShelfName shelf_name;
    ShelfId const shelf_id(1);
    std::string path = shelf_name.Path(shelf_id);
    ShelfFile shelf(path);
    EXPECT_EQ(NO_ERROR, shelf.Create(S_IRUSR|S_IWUSR, kShelfSize));

    int64_t* address = NULL;
    
    // write a value
    EXPECT_EQ(NO_ERROR, shelf.Open(O_RDWR));
    EXPECT_EQ(NO_ERROR, shelf.Map(NULL, kShelfSize, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address));
    fam_atomic_64_write(address, 123LL);
    EXPECT_EQ(NO_ERROR, shelf.Unmap(address, kShelfSize));
    EXPECT_EQ(NO_ERROR, shelf.Close());        

    // read it back
    EXPECT_EQ(NO_ERROR, shelf.Open(O_RDWR));
    EXPECT_EQ(NO_ERROR, shelf.Map(NULL, kShelfSize, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address));
    EXPECT_EQ(123LL, fam_atomic_64_read(address));
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

