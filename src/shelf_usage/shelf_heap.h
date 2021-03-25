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

#ifndef _NVMM_SHELF_HEAP_H_
#define _NVMM_SHELF_HEAP_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h> // for memset()
#include <assert.h> // for assert()
#include <string>

#include "nvmm/fam.h"
#include "nvmm/error_code.h"
#include "nvmm/global_ptr.h"

#include "common/common.h"
#include "shelf_mgmt/shelf_file.h"


namespace nvmm {

// assuming heap_size does not change once set
struct NvHeapLayout
{
    static void Create (void *address, size_t heap_size)
    {
        assert(address != NULL);
        assert(heap_size != 0);

        NvHeapLayout *layout = (NvHeapLayout*)address;
        layout->next_free = kMetadataSize;
        layout->heap_size = heap_size;
        memset(layout->data, 0, layout->heap_size);
        fam_persist((void*)(&layout->heap_size), kCacheLineSize*2+layout->heap_size);
        layout->magic_num = kMagicNum;        
        fam_persist((void*)(&layout->magic_num), kCacheLineSize);
    }

    static void Destroy (void *address)
    {
        assert(address != NULL);

        NvHeapLayout *layout = (NvHeapLayout*)address;
        size_t heap_size = layout->heap_size;                
        assert(layout->magic_num == kMagicNum);
        layout->next_free = 0;
        layout->heap_size = 0;
        memset(layout->data, 0, heap_size);
        fam_persist((void*)(&layout->heap_size), kCacheLineSize*2+layout->heap_size);
        layout->magic_num = 0;
        fam_persist((void*)(&layout->magic_num), kCacheLineSize);
    }

    static bool Verify (void *address)
    {
        assert(address != NULL);
        return fam_atomic_u64_read(&((NvHeapLayout*)address)->magic_num) == kMagicNum;
    }

    inline size_t Size()
    {   
        return heap_size;
    }

    // helper functions to access member variables from FAM
    inline Offset GetNextFree()
    {
        return (Offset)fam_atomic_u64_read((uint64_t*)&next_free);
    }

    inline Offset CASNextFree(Offset *address, Offset expected, Offset desired)
    {
        return (Offset)fam_atomic_u64_compare_and_store((uint64_t*)address,
                                                        (uint64_t)expected,
                                                        (uint64_t)desired);
    }
    
    // return absolute offset so that we can use 0 as NULL
    Offset Alloc(size_t size)
    {
        Offset expected_next_free, desired_next_free, ret;
    retry:
        expected_next_free = GetNextFree();
        desired_next_free = expected_next_free+round_up(size, kCacheLineSize);
        if (desired_next_free-kMetadataSize > heap_size)
        {
            ret = 0;
        }
        else
        {
            Offset actual_next_free = CASNextFree(&next_free, expected_next_free, desired_next_free);
            if (actual_next_free != expected_next_free)
            {
                goto retry;
            }
            else
            {
                ret = expected_next_free;
            }
        }
        return ret;
    }

    void Free(Offset offset)
    {
        return;
    }

    inline bool IsValid(Offset offset)
    {
        if (offset < kMetadataSize)
        {
            return false;
        }
        else
        {
            Offset relative_offset = offset - kMetadataSize;
            return relative_offset < (Offset)heap_size;
        }
    }    

    static uint64_t const kMagicNum = 684327; // nvheap
    static Offset const kMetadataSize = kCacheLineSize*3;
        
    uint64_t magic_num __attribute__((__aligned__(kCacheLineSize))); // must be equal to kMagicNum
    size_t heap_size __attribute__((__aligned__(kCacheLineSize))); // capacity of the heap (excluding metadata)
    Offset next_free __attribute__((__aligned__(kCacheLineSize))); // next free location
    char data[0];

};
        
class ShelfHeap
{
public:
    ShelfHeap() = delete;
    // the shelf file must already exist
    ShelfHeap(std::string pathname);    
    ShelfHeap(std::string pathname, ShelfId shelf_id);    
    ~ShelfHeap();

    ErrorCode Create(size_t size);        
    ErrorCode Destroy();    
    ErrorCode Verify();
    ErrorCode Recover();
    
    bool IsOpen() const
    {
        return is_open_;
    }
    
    ErrorCode Open();    
    ErrorCode Close();
    size_t Size();

    Offset Alloc(size_t size);
    void Free(Offset offset);

    bool IsValidOffset(Offset offset);
    bool IsValidPtr(void *addr);    
    void *OffsetToPtr(Offset offset) const;
    Offset PtrToOffset(void *addr);
    ErrorCode Map(Offset offset, size_t size, void *addr_hint, int prot, void **mapped_addr);
    ErrorCode Unmap(Offset offset, void *mapped_addr, size_t size);

private:
    ErrorCode OpenMapShelf(bool use_shelf_manager=false);
    ErrorCode UnmapCloseShelf(bool use_shelf_manager=false, bool unregister=false);  

    bool is_open_;
    ShelfFile shelf_;
    void *addr_;

    NvHeapLayout *layout_;    
    size_t size_; // size of the heap, which could be smaller than the size of the shelf
};

} // namespace nvmm
#endif
