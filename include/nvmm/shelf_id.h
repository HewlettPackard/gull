#ifndef _NVMM_SHELF_ID_H_
#define _NVMM_SHELF_ID_H_

#include <iostream>
#include <assert.h> // assert and static_assert
#include <functional> // std::hash

namespace nvmm{
    
// Shelf ID consists of Pool ID and Shelf Index (with in a pool)
// ShelfIdT is the internal storage type of the shelf id
// PoolIdT/ShelfIndexT is the external type of pool id/shelf index, whose size may be bigger than PoolIdTSize/ShelfIndexTSize
// ShelfIdTSize/PoolIdTSize/ShelfIndexTSize is the number of bits of ShelfIdT/PoolIdT/ShelfIndexT
template<class ShelfIdT, size_t ShelfIdTSize, class PoolIdT, size_t PoolIdTSize, class ShelfIndexT, size_t ShelfIndexTSize>
class ShelfIdClass
{
    static_assert(sizeof(ShelfIdT)*8 >= ShelfIdTSize, "Invalid ShelfIdT");
    static_assert(sizeof(PoolIdT)*8 >= PoolIdTSize, "Invalid PoolIdT");
    static_assert(sizeof(ShelfIndexT)*8 >= ShelfIndexTSize, "Invalid ShelfIndexT");
    static_assert(PoolIdTSize + ShelfIndexTSize <= ShelfIdTSize, "Invalid sizes");
    
public:
    // invalid shelf id: pool id == 0 && shelf index == 0
    static ShelfIdT const kInvalidShelfId = 0; 

    // max pool count and shelf count per pool
    static PoolIdT const kMaxPoolCount = 16;
    static ShelfIndexT const kMaxShelfCount = 16;
    
    ShelfIdClass()
        : shelf_id_{kInvalidShelfId}
    {
    }

    ShelfIdClass(ShelfIdT shelf_id)
        : shelf_id_{shelf_id}
    {
    }
    
    ShelfIdClass(PoolIdT pool_id, ShelfIndexT shelf_idx)
        : shelf_id_{EncodeShelfId(pool_id, shelf_idx)}
    {
        assert(pool_id<kMaxPoolCount && shelf_idx<kMaxShelfCount);        
    }

    ~ShelfIdClass()
    {
    }

    inline bool IsValid() const
    {
        return
            (GetPoolId()<kMaxPoolCount) &&
            (GetShelfIndex()<kMaxShelfCount) &&
            (shelf_id_ != kInvalidShelfId);
    }
    
    inline PoolIdT GetPoolId() const
    {
        return DecodePoolId(shelf_id_);
    }

    inline ShelfIndexT GetShelfIndex() const
    {
        return DecodeShelfIndex(shelf_id_);
    }

    inline ShelfIdT GetShelfId() const
    {
        return shelf_id_;
    }

    // operators
    friend std::ostream& operator<<(std::ostream& os, const ShelfIdClass& shelf_id)
    {
        os << "[" << ((uint64_t)shelf_id.GetPoolId()) << "_" << ((uint64_t)shelf_id.GetShelfIndex()) << "]";
        return os;
    }

    friend bool operator==(const ShelfIdClass &left, const ShelfIdClass &right)
    {
        return left.shelf_id_ == right.shelf_id_;
    }   

    friend bool operator!=(const ShelfIdClass &left, const ShelfIdClass &right)
    {
        return !(left == right);
    }   

    // hash and equal function for std::unordered_map<ShelfID, ...>
    struct Hash {
        unsigned long operator()(const ShelfIdClass& key) const
        {
            return std::hash<ShelfIdT>()(key.GetShelfId());
        }
    };

    struct Equal {
        bool operator()(const ShelfIdClass& lhs, const ShelfIdClass& rhs) const {
            return lhs.GetShelfId() == rhs.GetShelfId();
        }
    };
    
    
private:
    inline ShelfIdT EncodeShelfId(PoolIdT pool_id, ShelfIndexT shelf_idx) const
    {
        return (ShelfIdT)((((ShelfIdT)pool_id) << (ShelfIndexTSize)) + shelf_idx);
    }

    inline PoolIdT DecodePoolId(ShelfIdT shelf_id) const
    {
        return (PoolIdT)(((ShelfIdT)shelf_id) >> (ShelfIndexTSize));
    }

    inline ShelfIndexT DecodeShelfIndex(ShelfIdT shelf_id) const
    {
        return (ShelfIndexT)(((((ShelfIdT)1) << (ShelfIndexTSize)) - 1) & shelf_id);
    }
    
    ShelfIdT shelf_id_;
};

    
// internal type of ShelfId
using ShelfIdStorageType = uint8_t;
           
// PoolId
using PoolId = uint8_t;

// ShelfIndex
using ShelfIndex = uint8_t;

// ShelfId: 8-bit shelf id with 4-bit as pool id and 4-bit as shelf index
using ShelfId = ShelfIdClass<ShelfIdStorageType, 8, PoolId, 4, ShelfIndex, 4>;
    
} // namespace nvmm

#endif
