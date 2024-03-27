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

#include "nvmm/nvmm_fam_atomic.h"
#include <fcntl.h> // for O_RDWR
#include <gtest/gtest.h>
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"

#include "test_common/test.h"

#include "shelf_mgmt/shelf_file.h"
#include "shelf_mgmt/shelf_name.h"

#include "shelf_usage/ownership.h"

using namespace nvmm;

static size_t const kShelfSize = 128 * 1024 * 1024LLU; // 128 MB
static size_t const kItemCount = 64;

TEST(Ownership, Test) {
    ShelfName shelf_name;
    ShelfId const shelf_id(1);
    std::string path = shelf_name.Path(shelf_id);
    ShelfFile shelf(path);
    EXPECT_EQ(NO_ERROR, shelf.Create(S_IRUSR | S_IWUSR, kShelfSize));

    void *address = NULL;

    // mmap
    EXPECT_EQ(NO_ERROR, shelf.Open(O_RDWR));
    EXPECT_EQ(NO_ERROR, shelf.Map(NULL, kShelfSize, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, 0, (void **)&address));

    size_t length = kShelfSize / 2;
    loff_t offset = kShelfSize / 2;
    Ownership ownership((char *)address + offset, length);

    // create the ownership data structure
    EXPECT_EQ(NO_ERROR, ownership.Create(kItemCount));

    // open the ownership structure
    EXPECT_EQ(NO_ERROR, ownership.Open());

    for (size_t i = 0; i < ownership.Count(); i++) {
        EXPECT_FALSE(ownership.CheckItem(i));
        EXPECT_FALSE(ownership.ReleaseItem(i));
    }

    for (size_t i = 0; i < ownership.Count(); i++) {
        EXPECT_FALSE(ownership.CheckItem(i));
        EXPECT_TRUE(ownership.AcquireItem(i));
        EXPECT_TRUE(ownership.CheckItem(i));
    }

    for (size_t i = 0; i < ownership.Count(); i++) {
        EXPECT_TRUE(ownership.CheckItem(i));
        EXPECT_FALSE(ownership.AcquireItem(i));
        EXPECT_TRUE(ownership.CheckItem(i));
    }

    for (size_t i = 0; i < ownership.Count(); i++) {
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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment);
    return RUN_ALL_TESTS();
}
