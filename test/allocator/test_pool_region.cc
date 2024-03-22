/*
 *  (c) Copyright 2016-2021 Hewlett Packard Enterprise Development Company LP.
 *
 *  This software is available to you under a choice of one of two
 *  licenses. You may choose to be licensed under the terms of the 
 *  GNU Lesser General Public License Version 3, or (at your option)  
 *  later with exceptions included below, or under the terms of the  
 *  MIT license (Expat) available in COPYING file in the source tree.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As an exception, the copyright holders of this Library grant you permission
 *  to (i) compile an Application with the Library, and (ii) distribute the
 *  Application containing code generated by the Library and added to the
 *  Application during this compilation process under terms of your choice,
 *  provided you also meet the terms and conditions of the Application license.
 *
 */

#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED
#include <pthread.h>

#include <gtest/gtest.h>
#include "nvmm/nvmm_fam_atomic.h"

#include "nvmm/memory_manager.h"
#include "allocator/pool_region.h"

#include "test_common/test.h"

using namespace nvmm;

TEST(PoolRegion, CreateDestroyExist)
{
    PoolId pool_id = 1;
    size_t size = 128*1024*1024LLU; // 128 MB
    PoolRegion region(pool_id);
    
    // create a region
    EXPECT_EQ(NO_ERROR, region.Create(size));
    EXPECT_TRUE(region.Exist());
    EXPECT_EQ(POOL_FOUND, region.Create(size));

    // Destroy a region
    EXPECT_EQ(NO_ERROR, region.Destroy());
    EXPECT_FALSE(region.Exist());
    EXPECT_EQ(POOL_NOT_FOUND, region.Destroy());
}

TEST(PoolRegion, OpenCloseSize)
{
    PoolId pool_id = 1;
    size_t size = 128*1024*1024LLU; // 128 MB
    PoolRegion region(pool_id);

    // open a region that does not exist
    EXPECT_EQ(REGION_OPEN_FAILED, region.Open(O_RDWR));
    
    // create a region
    EXPECT_EQ(NO_ERROR, region.Create(size));

    // open the region
    EXPECT_EQ(NO_ERROR, region.Open(O_RDWR));
    
    // check its size
    EXPECT_EQ(size, region.Size());

    // close the region
    EXPECT_EQ(NO_ERROR, region.Close());
    
    // Destroy a region
    EXPECT_EQ(NO_ERROR, region.Destroy());
}

TEST(PoolRegion, MapUnmap)
{
    PoolId pool_id = 1;
    size_t size = 128*1024*1024LLU; // 128 MB
    int64_t* address = NULL;
    PoolRegion region(pool_id);

    // create a region
    EXPECT_EQ(NO_ERROR, region.Create(size));

    // write a value
    EXPECT_EQ(NO_ERROR, region.Open(O_RDWR));
    EXPECT_EQ(NO_ERROR, region.Map(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address));
    fam_atomic_64_write(address, 123LL);
    EXPECT_EQ(NO_ERROR, region.Unmap(address, size));
    EXPECT_EQ(NO_ERROR, region.Close());        

    // read it back
    EXPECT_EQ(NO_ERROR, region.Open(O_RDWR));
    EXPECT_EQ(NO_ERROR, region.Map(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address));
    EXPECT_EQ(123LL, fam_atomic_64_read(address));
    EXPECT_EQ(NO_ERROR, region.Unmap(address, size));
    EXPECT_EQ(NO_ERROR, region.Close());        
              
    // destroy the region
    EXPECT_EQ(NO_ERROR, region.Destroy());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment);
    return RUN_ALL_TESTS();
}
