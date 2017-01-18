#include <mutex>
#include <map>
#include <unordered_map>
#include <stddef.h>

#include <fcntl.h>
#include <sys/mman.h>

#include "nvmm/shelf_id.h"

#include "nvmm/log.h"
#include "shelf_mgmt/shelf_manager.h"
#include "shelf_mgmt/shelf_file.h"

namespace nvmm {

std::unordered_map<ShelfId, std::pair<void *, size_t>, ShelfId::Hash, ShelfId::Equal> ShelfManager::map_;
std::map<void *, std::pair<ShelfId, size_t>> ShelfManager::reverse_map_;
std::mutex ShelfManager::map_mutex_;
    
void *ShelfManager::RegisterShelf(ShelfId shelf_id, void *base, size_t length)
{
    auto entry = std::make_pair(shelf_id, std::make_pair(base, length));
    auto result = map_.insert(entry);
    if (result.second == true)
    {
        LOG(trace) << "RegisterShelf: mapping registered";
        auto reverse_entry = std::make_pair(base, std::make_pair(shelf_id,length));
        auto reverse_result = reverse_map_.insert(reverse_entry);
        assert(reverse_result.second == true);
        return base;
    }
    else
    {
        LOG(trace) << "RegisterShelf: existing mapping";
        assert(result.first->second.second == length);
        return result.first->second.first;
    }
}

void *ShelfManager::UnregisterShelf(ShelfId shelf_id)
{
    auto result = map_.find(shelf_id);
    if (result == map_.end())
    {
        LOG(trace) << "UnregisterShelf: mapping not found";
        return NULL;
    }
    else
    {
        void *ret = result->second.first;
        (void)map_.erase(result);
        LOG(trace) << "UnregisterShelf: mapping unregistered";
        auto reverse_result = reverse_map_.find(ret);
        assert(reverse_result != reverse_map_.end());
        (void)reverse_map_.erase(reverse_result);
        return ret;
    }
}


void *ShelfManager::LookupShelf(ShelfId shelf_id)
{
    auto result = map_.find(shelf_id);
    if (result == map_.end())
    {
        LOG(trace) << "LookupShelf: mapping not found";
        return NULL;
    }
    else
    {
        LOG(trace) << "LookupShelf: mapping found";
        return result->second.first;
    }
}
    
void *ShelfManager::FindBase(ShelfId shelf_id)
{
    ShelfManager::Lock();
    void *ret = LookupShelf(shelf_id);
    ShelfManager::Unlock();
    return ret;
}

void *ShelfManager::FindBase(std::string path, ShelfId shelf_id)
{
    void *addr;
    ShelfManager::Lock();
    auto result = map_.find(shelf_id);
    if (result != map_.end())
    {
        LOG(trace) << "FindBase: mapping found";
        ShelfManager::Unlock();
        return result->second.first;
    }

    LOG(trace) << "FindBase: mapping not found";    
    ShelfFile shelf(path);    
    size_t length = shelf.Size();
    int prot = PROT_READ|PROT_WRITE;
    int flags = MAP_SHARED;
    loff_t offset = 0;

    ErrorCode ret = shelf.Open(O_RDWR);
    if (ret != NO_ERROR)
    {
        ShelfManager::Unlock();
        return NULL;
    }
    ret = shelf.Map(NULL, length, prot, flags, offset, &addr);
    if (ret == NO_ERROR)
    {
        void *actual_addr = ShelfManager::RegisterShelf(shelf_id, addr, length);
        assert(actual_addr == addr);
        ShelfManager::Unlock();
        (void)shelf.Close();
        return addr;
    }
    else
    {
        ShelfManager::Unlock();
        (void)shelf.Close();
        return NULL;
    }    
}
    
ShelfId ShelfManager::FindShelf(void *ptr, void *&base)
{
    // TODO: optimization
    for(auto it : reverse_map_)
    {
        base = it.first;
        size_t length = it.second.second;
        if ((uintptr_t)ptr >= (uintptr_t)base && (uintptr_t)ptr < ((uintptr_t)base+length))
        {
            LOG(trace) << "FindShelf: mapping found";
            return it.second.first;
        }
    }
    LOG(trace) << "FindShelf: mapping not found";
    return ShelfId(); // an invalid shelf id
}

void ShelfManager::Reset()
{
    for(auto it=map_.begin(); it!=map_.end(); it++)
    {
        void *base = it->second.first;
        size_t length = it->second.second;
        ShelfFile::Unmap(base, length, true);
    }
    map_.clear();
    reverse_map_.clear();
}
    
void ShelfManager::Lock()
{
    map_mutex_.lock();
}

void ShelfManager::Unlock()
{
    map_mutex_.unlock();
}

} // namespace nvmm
