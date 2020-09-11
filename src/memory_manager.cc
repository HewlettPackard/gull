/*
 *  (c) Copyright 2016-2017 Hewlett Packard Enterprise Development Company LP.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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

#include <memory>
#include <iostream>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <fcntl.h> // for O_RDWR
#include <sys/mman.h> // for PROT_READ, PROT_WRITE, MAP_SHARED
#include <unistd.h> // for getpagesize()
#include <string>
#include <boost/filesystem.hpp>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h" // for PoolId
#include "nvmm/global_ptr.h" // for GlobalPtr
#include "nvmm/heap.h"
#include "nvmm/region.h"

#include "nvmm/log.h"
#include "nvmm/fam.h"
#include "nvmm/memory_manager.h"

#include "common/config.h"

#include "common/root_shelf.h"
#include "common/epoch_shelf.h"
#include "shelf_mgmt/shelf_manager.h"
#include "allocator/pool_region.h"
#ifdef ZONE
#include "allocator/epoch_zone_heap.h"
#else
#include "allocator/dist_heap.h"
#endif

namespace nvmm {

// void Init(std::string config_file) {
//     int ret = config.LoadConfigFile(config_file);
//     if(ret!=0) {
//         std::cout << "NVMM: failed to load the config_file at " << config_file << std::endl;
//         return;
//     }
//     if (boost::filesystem::exists(config.ShelfBase) == false)
//     {
//         if(boost::filesystem::create_directory(config.ShelfBase) == false) {
//             std::cout << "NVMM: failed to create shelf base dir at " << config.ShelfBase << std::endl;
//         }
//     }
// }

// TODO: remove these global instances and static classes
// memory manager and epoch manager
// Config config in config.cc
// ShelfManager in shelf_manager.h
void StartNVMM(std::string base, std::string user) {
    if(!base.empty() || !user.empty())
        config=Config(base, user);

    if (boost::filesystem::exists(config.ShelfBase) == false)
    {
        if(boost::filesystem::create_directory(config.ShelfBase) == false) {
            std::cout << "NVMM: failed to create shelf base dir at " << config.ShelfBase << std::endl;
        }
    }

    // Check if shelf base exists
    boost::filesystem::path shelf_base_path = boost::filesystem::path(config.ShelfBase);
    if (boost::filesystem::exists(shelf_base_path) == false)
    {
        LOG(fatal) << "NVMM: LFS/tmpfs does not exist?" << config.ShelfBase;
        exit(1);
    }

    // create a root shelf for MemoryManager if it does not exist
    RootShelf root_shelf(config.RootShelfPath);
    if(root_shelf.Exist() == false)
    {
        ErrorCode ret = root_shelf.Create();
        if (ret!=NO_ERROR && ret != SHELF_FILE_FOUND)
        {
            LOG(fatal) << "NVMM: Failed to create the root shelf file " << config.RootShelfPath;
            exit(1);
        }
    }

    // create a epoch shelf for EpochManager if it does not exist
    EpochShelf epoch_shelf(config.EpochShelfPath);
    if(epoch_shelf.Exist() == false)
    {
        ErrorCode ret = epoch_shelf.Create();
        if (ret!=NO_ERROR && ret != SHELF_FILE_FOUND)
        {
            LOG(fatal) << "NVMM: Failed to create the epoch shelf file " << config.EpochShelfPath;
            exit(1);
        }
    }
}

void ResetNVMM(std::string base, std::string user) {
    if(!base.empty() || !user.empty())
        config=Config(base, user);

    std::string cmd;

    // remove memory manager root shelf
    cmd = std::string("exec rm -f ") + config.RootShelfPath + " > /dev/null";
    (void)system(cmd.c_str());

    // remove epoch manager root shelf
    cmd = std::string("exec rm -f ") + config.EpochShelfPath + " > /dev/null";
    (void)system(cmd.c_str());

    // remove previous files in shelf base
    cmd = std::string("exec rm -f ") + config.ShelfBase + "/" + config.ShelfUser + "_NVMM_Shelf* > /dev/null";
    (void)system(cmd.c_str());
}

void RestartNVMM(std::string base, std::string user) {
    EpochManager::GetInstance()->Stop();
    MemoryManager::GetInstance()->Stop();

    // DO NOT remove previous shelves
    //ResetNVMM(); // reset with existing config
    StartNVMM(base, user); // start using the new config

    MemoryManager::GetInstance()->Start();
    EpochManager::GetInstance()->Start();
}


/*
 * Internal implementation of MemoryManager
 */
class MemoryManager::Impl_
{
public:
    Impl_()
        : is_ready_(false), root_shelf_(config.RootShelfPath), locks_(NULL), types_(NULL)
    {
    }

    ~Impl_()
    {
    }

    ErrorCode Init();
    ErrorCode Final();

    ErrorCode MapPointer(GlobalPtr ptr, size_t size,
                         void *addr_hint, int prot, int flags, void **mapped_addr);
    ErrorCode UnmapPointer(GlobalPtr ptr, void *mapped_addr, size_t size);

    void *GlobalToLocal(GlobalPtr ptr);
    GlobalPtr LocalToGlobal(void *addr);

    ErrorCode CreateHeap(PoolId id, size_t shelf_size, size_t min_alloc_size, mode_t mode);
    ErrorCode DestroyHeap(PoolId id);
    ErrorCode FindHeap(PoolId id, Heap **heap);
    Heap *FindHeap(PoolId id);

    ErrorCode CreateRegion(PoolId id, size_t size);
    ErrorCode DestroyRegion(PoolId id);
    ErrorCode FindRegion(PoolId id, Region **region);
    Region *FindRegion(PoolId id);
    void *GetRegionIdBitmapAddr();
    GlobalPtr GetMetadataRegionRootPtr(int type);
    GlobalPtr SetMetadataRegionRootPtr(int type, GlobalPtr);
    GlobalPtr GetATLRegionRootPtr(int type);
    GlobalPtr SetATLRegionRootPtr(int type, GlobalPtr);


private:
    enum PoolType {
        NONE=0,
        REGION,
        HEAP
    }; // __attribute__((__aligned__(64))) does not work for enum type???

    struct PoolTypeEntry {
        PoolType type;
    } __attribute__((__aligned__(64)));

    // multi-process/multi-node
    // TODO: NOT resilient to crashes, need the epoch system
    inline void Lock(PoolId pool_id)
    {
        locks_[pool_id].lock();
    }

    inline void Unlock(PoolId pool_id)
    {
        locks_[pool_id].unlock();
    }

    inline bool TryLock(PoolId pool_id)
    {
        return locks_[pool_id].trylock();
    }

    inline void SetType(PoolId pool_id, PoolType pool_type)
    {
        fam_atomic_u64_write((uint64_t*)&types_[pool_id], (uint64_t)pool_type);
    }

    inline PoolType GetType(PoolId pool_id)
    {
        return (PoolType)fam_atomic_u64_read((uint64_t*)&types_[pool_id]);
    }

    bool is_ready_;
    RootShelf root_shelf_;
    nvmm_fam_spinlock* locks_; // an array of fam_spinlock
    PoolTypeEntry *types_; // store pool types
    void *bmap_addr_; // store bitmap start address for region id management
    uint64_t *metadata_regionid_root_; // Store metadata region id's globalptr
    uint64_t *metadata_regionname_root_;// Store metadata region name's globalptr
    uint64_t *atl_regionid_root_; // Store ATL region id's globalptr
    uint64_t *atl_regionname_root_;// Store ATL region name's globalptr
};

ErrorCode MemoryManager::Impl_::Init()
{
    boost::filesystem::path shelf_base_path = boost::filesystem::path(config.ShelfBase);
    if (boost::filesystem::exists(shelf_base_path) == false)
    {
        LOG(fatal) << "NVMM: LFS/tmpfs does not exist?" << config.ShelfBase;
        exit(1);
    }

    if (root_shelf_.Exist() == false)
    {
        LOG(fatal) << "NVMM: Root shelf does not exist?" << config.RootShelfPath;
        exit(1);
    }

    if (root_shelf_.Open() != NO_ERROR)
    {
        LOG(fatal) << "NVMM: Root shelf open failed..." << config.RootShelfPath;
        exit(1);
    }

    locks_ = (nvmm_fam_spinlock*)root_shelf_.Addr();
    types_ = (PoolTypeEntry*)((char*)root_shelf_.Addr() + ShelfId::kMaxPoolCount*sizeof(nvmm_fam_spinlock));
    // get the addr to be used by bitmap for region id management
    // Total size used by the bitmap will be kMaxPoolCount/8 bytes.
    bmap_addr_ = (void*)(types_ + ShelfId::kMaxPoolCount*sizeof(PoolTypeEntry));
    // Region Id and name rootptr for metadata
    uint64_t *next_addr = (uint64_t *)((char *)bmap_addr_+ShelfId::kMaxPoolCount/(sizeof (uint64_t)));
    uint64_t addr  = (uint64_t)next_addr;
    addr = (addr + (0x7)) & ~7UL;  // Round up to 8-byte boundary
    next_addr = (uint64_t *)addr;
    metadata_regionid_root_ = next_addr+1;
    metadata_regionname_root_ = metadata_regionid_root_ + 1;
    //Region Id and name rootptr for ATL
    atl_regionid_root_ = metadata_regionname_root_ + 1;
    atl_regionname_root_ = atl_regionid_root_ + 1;

    is_ready_ = true;
    return NO_ERROR;
}

ErrorCode MemoryManager::Impl_::Final()
{
    ErrorCode ret = root_shelf_.Close();
    if (ret!=NO_ERROR)
    {
        LOG(fatal) << "NVMM: Root shelf close failed" << config.RootShelfPath;
        exit(1);
    }

    is_ready_ = false;
    return NO_ERROR;
}

ErrorCode MemoryManager::Impl_::CreateRegion(PoolId id, size_t size)
{
    assert(is_ready_ == true);
    assert(id > 0);
    ErrorCode ret = NO_ERROR;
    Lock(id);
    if (GetType(id)!=PoolType::NONE)
    {
        Unlock(id);
        LOG(error) << "MemoryManager: the given id (" << (uint64_t)id << ") is in use";
        return ID_FOUND;
    }
    PoolRegion pool_region(id);
    ret = pool_region.Create(size);
    if (ret == NO_ERROR)
    {
        SetType(id, PoolType::REGION);
        Unlock(id);
        return ret;
    }
    Unlock(id);
    if (ret == POOL_FOUND)
    {
        LOG(error) << "MemoryManager: the given id (" << (uint64_t)id << ") is in use";
        return ID_FOUND;
    }
    else
    {
        LOG(fatal) << "MemoryManager: error " << ret;
        return ID_FOUND;
    }
}

// TODO: verify the pool is indeed a region
ErrorCode MemoryManager::Impl_::DestroyRegion(PoolId id)
{
    assert(is_ready_ == true);
    assert(id > 0);
    ErrorCode ret = NO_ERROR;
    Lock(id);
    if (GetType(id)!=PoolType::REGION)
    {
        Unlock(id);
        LOG(error) << "MemoryManager: region of the given id (" << (uint64_t)id << ") is not found";
        return ID_NOT_FOUND;
    }
    PoolRegion pool_region(id);
    ret = pool_region.Destroy();
    if (ret == NO_ERROR)
    {
        SetType(id, PoolType::NONE);
        Unlock(id);
        return NO_ERROR;
    }
    Unlock(id);
    if (ret == POOL_NOT_FOUND)
    {
        LOG(error) << "MemoryManager: region of the given id (" << (uint64_t)id << ") is not found";
        return ID_NOT_FOUND;
    }
    else
    {
        LOG(fatal) << "MemoryManager: error " << ret;
        return ID_NOT_FOUND;
    }
}

ErrorCode MemoryManager::Impl_::FindRegion(PoolId id, Region **region)
{
    assert(is_ready_ == true);
    assert(id > 0);
    Lock(id);
    if (GetType(id)!=PoolType::REGION)
    {
        Unlock(id);
        LOG(error) << "MemoryManager: region of the given id (" << (uint64_t)id << ") is not found";
        return ID_NOT_FOUND;
    }
    // TODO: use smart poitner or unique pointer?
    PoolRegion *pool_region = new PoolRegion(id);
    assert(pool_region != NULL);
    Unlock(id);
    if (pool_region->Exist() == true)
    {
        *region = (Region*)pool_region;
        return NO_ERROR;
    }
    else
    {
        LOG(error) << "MemoryManager: region of the given id (" << (uint64_t)id << ") is not found";
        delete pool_region;
        return ID_NOT_FOUND;
    }
}

Region *MemoryManager::Impl_::FindRegion(PoolId id)
{
    assert(is_ready_ == true);
    assert(id > 0);
    Region *ret = NULL;
    (void)FindRegion(id, &ret);
    return ret;
}

ErrorCode MemoryManager::Impl_::CreateHeap(PoolId id, size_t size, size_t min_alloc_size, mode_t mode)
{
    assert(is_ready_ == true);
    assert(id > 0);
    ErrorCode ret = NO_ERROR;
    Lock(id);
    if (GetType(id)!=PoolType::NONE)
    {
        Unlock(id);
        LOG(error) << "MemoryManager: the given id (" << (uint64_t)id << ") is in use";
        return ID_FOUND;
    }
#ifdef ZONE
    EpochZoneHeap heap(id);
#else
    DistHeap heap(id);
#endif
    ret = heap.Create(size, min_alloc_size, mode);
    if (ret == NO_ERROR)
    {
        SetType(id, PoolType::HEAP);
        Unlock(id);
        return ret;
    }
    Unlock(id);
    if (ret == POOL_FOUND)
    {
        LOG(error) << "MemoryManager: the given id (" << (uint64_t)id << ") is in use";
        return ID_FOUND;
    }
    else
    {
        LOG(fatal) << "MemoryManager: error " << ret;
        return ID_FOUND;
    }
}

ErrorCode MemoryManager::Impl_::DestroyHeap(PoolId id)
{
    assert(is_ready_ == true);
    assert(id > 0);
    ErrorCode ret = NO_ERROR;
    Lock(id);
    if (GetType(id)!=PoolType::HEAP)
    {
        Unlock(id);
        LOG(error) << "MemoryManager: heap of the given id (" << (uint64_t)id << ") is not found";
        return ID_NOT_FOUND;
    }
#ifdef ZONE
    EpochZoneHeap heap(id);
#else
    //PoolHeap heap(id);
    DistHeap heap(id);
#endif
    ret = heap.Destroy();
    if (ret == NO_ERROR)
    {
        SetType(id, PoolType::NONE);
        Unlock(id);
        return NO_ERROR;
    }
    Unlock(id);
    if (ret == POOL_NOT_FOUND)
    {
        LOG(error) << "MemoryManager: heap of the given id (" << (uint64_t)id << ") is not found";
        return ID_NOT_FOUND;
    }
    else
    {
        LOG(fatal) << "MemoryManager: error " << ret;
        return ID_NOT_FOUND;
    }
}

ErrorCode MemoryManager::Impl_::FindHeap(PoolId id, Heap **heap)
{
    assert(is_ready_ == true);
    assert(id > 0);
    Lock(id);
    if (GetType(id)!=PoolType::HEAP)
    {
        Unlock(id);
        LOG(error) << "MemoryManager: heap of the given id (" << (uint64_t)id << ") is not found";
        return ID_NOT_FOUND;
    }
    // TODO: use smart poitner or unique pointer?
#ifdef ZONE
    EpochZoneHeap *heap_ = new EpochZoneHeap(id);
#else
    DistHeap *heap_ = new DistHeap(id);
#endif
    assert(heap_ != NULL);
    Unlock(id);
    if(heap_->Exist() == true)
    {
        *heap = (Heap*)heap_;
        return NO_ERROR;
    }
    else
    {
        LOG(error) << "MemoryManager: heap of the given id (" << (uint64_t)id << ") is not found";
        return ID_NOT_FOUND;
    }
}

Heap *MemoryManager::Impl_::FindHeap(PoolId id)
{
    assert(is_ready_ == true);
    assert(id > 0);
    Heap *ret = NULL;
    (void)FindHeap(id, &ret);
    return ret;
}

ErrorCode MemoryManager::Impl_::MapPointer(GlobalPtr ptr, size_t size,
                                    void *addr_hint, int prot, int flags, void **mapped_addr)
{
    assert(is_ready_ == true);

    if (ptr.IsValid() == false)
    {
        LOG(error) << "MemoryManager: Invalid Global Pointer: " << ptr;
        return INVALID_PTR;
    }

    ErrorCode ret = NO_ERROR;
    ShelfId shelf_id = ptr.GetShelfId();
    PoolId pool_id = shelf_id.GetPoolId();
    if (pool_id == 0)
    {
        LOG(error) << "MemoryManager: Invalid Global Pointer: " << ptr;
        return INVALID_PTR;
    }
    ShelfIndex shelf_idx = shelf_id.GetShelfIndex();
    Offset offset = ptr.GetOffset();

    // handle alignment
    int page_size = getpagesize();
    off_t aligned_start = offset - offset % page_size;
    assert(aligned_start % page_size == 0);
    off_t aligned_end = (offset + size) + (page_size - (offset + size) % page_size);
    assert(aligned_end % page_size == 0);
    size_t aligned_size = aligned_end - aligned_start;
    assert(aligned_size % page_size == 0);

    void *aligned_addr = NULL;

    // TODO: this is way too costly
    // open the pool
    Pool pool(pool_id);
    ret = pool.Open(false);
    if (ret != NO_ERROR)
    {
        return MAP_POINTER_FAILED;
    }

    std::string shelf_path;
    ret = pool.GetShelfPath(shelf_idx, shelf_path);
    if (ret != NO_ERROR)
    {
        return MAP_POINTER_FAILED;
    }

    ShelfFile shelf(shelf_path);

    // open the shelf file
    ret = shelf.Open(O_RDWR);
    if (ret != NO_ERROR)
    {
        return MAP_POINTER_FAILED;
    }

    // only mmap offset + size
    ret = shelf.Map(addr_hint, aligned_size, PROT_READ|PROT_WRITE, MAP_SHARED, aligned_start,
                    &aligned_addr, false);
    if (ret != NO_ERROR)
    {
        return MAP_POINTER_FAILED;
    }

    // close the shelf file
    ret = shelf.Close();
    if (ret != NO_ERROR)
    {
        return MAP_POINTER_FAILED;
    }

    // close the pool
    ret = pool.Close(false);
    if (ret != NO_ERROR)
    {
        return MAP_POINTER_FAILED;
    }

    *mapped_addr = (void*)((char*)aligned_addr + offset % page_size);

    LOG(trace) << "MapPointer: path " << shelf.GetPath()
               << " offset " << aligned_start << " size " << aligned_size
               << " aligned ptr " << (void*)aligned_addr
               << " returned ptr " << (void*)(*mapped_addr);

    return ret;

}

ErrorCode MemoryManager::Impl_::UnmapPointer(GlobalPtr ptr, void *mapped_addr, size_t size)
{
    assert(is_ready_ == true);
    Offset offset = ptr.GetOffset();
    int page_size = getpagesize();
    off_t aligned_start = offset - offset % page_size;
    assert(aligned_start % page_size == 0);
    off_t aligned_end = (offset + size) + (page_size - (offset + size) % page_size);
    assert(aligned_end % page_size == 0);
    size_t aligned_size = aligned_end - aligned_start;
    assert(aligned_size % page_size == 0);
    void *aligned_addr = (void*)((char*)mapped_addr - offset % page_size);
    LOG(trace) << "UnmapPointer: path " << " offset " << aligned_start << " size " << aligned_size
               << " aligned ptr " << (void*)aligned_addr
               << " input ptr " << (void*)mapped_addr;
    return ShelfFile::Unmap(aligned_addr, aligned_size, false);
}

void *MemoryManager::Impl_::GlobalToLocal(GlobalPtr ptr)
{
    assert(is_ready_ == true);

    if (ptr.IsValid() == false)
    {
        LOG(error) << "MemoryManager: Invalid Global Pointer: " << ptr;
        return NULL;
    }

    ErrorCode ret = NO_ERROR;
    ShelfId shelf_id = ptr.GetShelfId();
    Offset offset = ptr.GetOffset();
    void *addr = ShelfManager::FindBase(shelf_id);
    if (addr == NULL)
    {
        // slow path
        // first time accessing this shelf
        PoolId pool_id = shelf_id.GetPoolId();
        if (pool_id == 0)
        {
            LOG(error) << "MemoryManager: Invalid Global Pointer: " << ptr;
            return NULL;
        }
        // TODO: this is way too expensive
        // open the pool
        Pool pool(pool_id);
        ret = pool.Open(false);
        if (ret != NO_ERROR)
        {
            return NULL;
        }

        ShelfIndex shelf_idx = shelf_id.GetShelfIndex();
        std::string shelf_path;
        ret = pool.GetShelfPath(shelf_idx, shelf_path);
        if (ret != NO_ERROR)
        {
            return NULL;
        }

        addr = ShelfManager::FindBase(shelf_path, shelf_id);

        // close the pool
        (void) pool.Close(false);
    }

    if (addr != NULL)
    {
        addr = (void*)((char*)addr+offset);
        LOG(trace) << "GetLocalPtr: global ptr" << ptr
                   << " offset " << offset
                   << " returned ptr " << (uintptr_t)addr;
    }

    return addr;
}

// TODO(zone): how to make this work for zone ptr?
GlobalPtr MemoryManager::Impl_::LocalToGlobal(void *addr)
{
    void *base = NULL;
    ShelfId shelf_id = ShelfManager::FindShelf(addr, base);
    if (shelf_id.IsValid() == false)
    {
        LOG(error) << "GetGlobalPtr failed";
        return GlobalPtr(); // return an invalid global pointer
    }
    else
    {
        Offset offset = (uintptr_t)addr - (uintptr_t)base;
        GlobalPtr global_ptr = GlobalPtr(shelf_id, offset);
        LOG(trace) << "GetGlobalPtr: local ptr " << (uintptr_t)addr
                   << " offset " << offset
                   << " returned ptr " << global_ptr;
        return global_ptr;
    }
}

void *MemoryManager::Impl_::GetRegionIdBitmapAddr()
{
    return bmap_addr_;
}

GlobalPtr MemoryManager::Impl_::GetMetadataRegionRootPtr(int type){
    if (type == METADATA_REGION_ID) {
        return GlobalPtr(fam_atomic_u64_read(metadata_regionid_root_));
    } else if (type == METADATA_REGION_NAME) {
        return GlobalPtr(fam_atomic_u64_read(metadata_regionname_root_));
    }
    return GlobalPtr(); // return an invalid global pointer
}

GlobalPtr MemoryManager::Impl_::SetMetadataRegionRootPtr(int type, GlobalPtr regionPtr){
    if (type == METADATA_REGION_ID) {
        uint64_t regptr = regionPtr.ToUINT64();
        // update metadata_regionid_root_ only if it doesn't have any value
        uint64_t resultptr = fam_atomic_u64_compare_and_store(metadata_regionid_root_, 0, regptr);
        if (resultptr == 0) {
             return regionPtr;
        } else {
             return GlobalPtr(resultptr);
        }
    } else if (type == METADATA_REGION_NAME) {
        uint64_t regptr = regionPtr.ToUINT64();
        // update metadata_regionname_root_ only if it doesn't have any value
        uint64_t resultptr = fam_atomic_u64_compare_and_store(metadata_regionname_root_, 0, regptr);
        if (resultptr == 0) {
             return regionPtr;
        } else {
             return GlobalPtr(resultptr);
        }
    }
    return GlobalPtr(); // return an invalid global pointer;
}

GlobalPtr MemoryManager::Impl_::GetATLRegionRootPtr(int type){
    if (type == ATL_REGION_ID) {
        return GlobalPtr(fam_atomic_u64_read(atl_regionid_root_));
    } else if (type == ATL_REGION_NAME) {
        return GlobalPtr(fam_atomic_u64_read(atl_regionname_root_));
    }
    return GlobalPtr(); // return an invalid global pointer
}

GlobalPtr MemoryManager::Impl_::SetATLRegionRootPtr(int type, GlobalPtr regionPtr){
    if (type == ATL_REGION_ID) {
        uint64_t regptr = regionPtr.ToUINT64();
        // update atl_regionid_root_ only if it doesn't have any value
        uint64_t resultptr = fam_atomic_u64_compare_and_store(atl_regionid_root_, 0, regptr);
        if (resultptr == 0) {
             return regionPtr;
        } else {
             return GlobalPtr(resultptr);
        }
    } else if (type == ATL_REGION_NAME) {
        uint64_t regptr = regionPtr.ToUINT64();
        // update atl_regionname_root_ only if it doesn't have any value
        uint64_t resultptr = fam_atomic_u64_compare_and_store(atl_regionname_root_, 0, regptr);
        if (resultptr == 0) {
             return regionPtr;
        } else {
             return GlobalPtr(resultptr);
        }
    }
    return GlobalPtr(); // return an invalid global pointer;
}

/*
 * Public APIs of MemoryManager
 */

// thread-safe Singleton pattern with C++11
// see http://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/
MemoryManager *MemoryManager::GetInstance()
{
    static MemoryManager instance;
    return &instance;
}

MemoryManager::MemoryManager()
{
    Start();
}

MemoryManager::~MemoryManager()
{
    Stop();
}

void MemoryManager::Stop()
{
    ErrorCode ret = pimpl_->Final();
    assert(ret == NO_ERROR);
    if(pimpl_)
        delete pimpl_;
}

void MemoryManager::Start()
{
    pimpl_ = new Impl_;
    assert(pimpl_);
    ErrorCode ret = pimpl_->Init();
    assert(ret == NO_ERROR);
}

ErrorCode MemoryManager::CreateRegion(PoolId id, size_t size)
{
    return pimpl_->CreateRegion(id, size);
}

ErrorCode MemoryManager::DestroyRegion(PoolId id)
{
    return pimpl_->DestroyRegion(id);
}

ErrorCode MemoryManager::FindRegion(PoolId id, Region **region)
{
    return pimpl_->FindRegion(id, region);
}

Region *MemoryManager::FindRegion(PoolId id)
{
    return pimpl_->FindRegion(id);
}

ErrorCode MemoryManager::CreateHeap(PoolId id, size_t size, size_t min_alloc_size, mode_t mode)
{
    return pimpl_->CreateHeap(id, size, min_alloc_size, mode);
}

ErrorCode MemoryManager::DestroyHeap(PoolId id)
{
    return pimpl_->DestroyHeap(id);
}

ErrorCode MemoryManager::FindHeap(PoolId id, Heap **heap)
{
    return pimpl_->FindHeap(id, heap);
}

Heap *MemoryManager::FindHeap(PoolId id)
{
    return pimpl_->FindHeap(id);
}

ErrorCode MemoryManager::MapPointer(GlobalPtr ptr, size_t size,
                                    void *addr_hint, int prot, int flags, void **mapped_addr)
{
    return pimpl_->MapPointer(ptr, size, addr_hint, prot, flags, mapped_addr);
}

ErrorCode MemoryManager::UnmapPointer(GlobalPtr ptr, void *mapped_addr, size_t size)
{
    return pimpl_->UnmapPointer(ptr, mapped_addr, size);
}

void *MemoryManager::GlobalToLocal(GlobalPtr ptr)
{
    return pimpl_->GlobalToLocal(ptr);
}

GlobalPtr MemoryManager::LocalToGlobal(void *addr)
{
    return pimpl_->LocalToGlobal(addr);
}

void *MemoryManager::GetRegionIdBitmapAddr()
{
    return pimpl_->GetRegionIdBitmapAddr();
}

GlobalPtr MemoryManager::GetMetadataRegionRootPtr(int type){
    return pimpl_->GetMetadataRegionRootPtr(type);
}

GlobalPtr MemoryManager::SetMetadataRegionRootPtr(int type, GlobalPtr regionRoot){
    return pimpl_->SetMetadataRegionRootPtr(type, regionRoot);
}

} // namespace nvmm
