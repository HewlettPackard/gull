# Non-Volatile Memory Manager (NVMM)
## Description

Non-Volatile Memory Manager (NVMM) is a library written in C++ that provides simple abstractions
for accessing and allocating Non-Volatile Memory (NVM) from Fabric-Attached Memory (FAM). Built
on top of the Librarian File System (LFS), NVMM manages shelves from LFS and assigns them unique
shelf IDs so that it exposes a persistent address space (in the form of shelf ID and offset) that
enables universal access and global sharing. Furthermore, NVMM presents one or more shelves as a
NVM pool and builds abstractions on top of pools to support various NVM use styles. Currently
NVMM supports direct memory mapping through **Region** (mmap/unmap) and finer-grained allocation
through **Heap** (alloc/free).

A key feature of NVMM is its *multi-process* and *multi-node* support. NVMM is specifically designed
to work in cache-incoherent multi-node environment like The Machine. The NVM pool layer permits
concurrent shelf creation and deletion (within a pool) from different processes on different
nodes. The built-in heap implementation enables any process from any node to allocate and free globally
shared NVM transparently.

## Master Source

https://github.com/HewlettPackard/gull

## Maturity

NVMM is still in alpha state. The basic functionalities are working, but performance is not
optimized.

NVMM runs on both NUMA and FAME ([Fabric-Attached Memory
 Emulation](https://github.com/HewlettPackard/mdc-toolkit/blob/master/guide-FAME.md)) systems.

## Dependencies

- Install additional packages

  ```
  $ sudo apt-get install build-essential cmake libboost-all-dev libyaml-cpp-dev
  ```

  You can specify custom path for libboost by setting an environment variable BOOST_ROOT
  ```
  $ export BOOST_ROOT=/opt/usr/local
  ```

- Install libpmem

  ```
  $ sudo apt-get install autoconf pkg-config doxygen graphviz
  $ git clone https://github.com/FabricAttachedMemory/nvml.git
  $ cd nvml
  $ make
  $ sudo make install

  You can provide custom directory prefix for installation using DESTDIR variable. For example, to install to directory /opt

  $ sudo make install DESTDIR=/opt
  ```

## Build & Test

1. Download the source code:

 ```
 $ git clone https://github.com/HewlettPackard/gull.git
 ```

2. Change into the source directory (assuming the code is at directory $NVMM):

 ```
 $ cd $NVMM
 ```

3. Build

 On CC-NUMA systems (default):

 ```
 $ mkdir build
 $ cd build
 $ cmake .. -DFAME=OFF
 $ make
 ```
 Please set CMAKE_PREFIX_PATH to point to nvml install directory if it is not in
 system path.

 On FAME:

 ```
 $ mkdir build
 $ cd build
 $ cmake .. -DFAME=ON
 $ make
 ```

 The default build type is Release. To switch between Release and Debug:
 ```
 $ cmake .. -DCMAKE_BUILD_TYPE=Release
 $ cmake .. -DCMAKE_BUILD_TYPE=Debug
 ```

 Other build options include:
 - LOG: enable (ON) or disable (OFF, default) logging
 - USE_FAM_ATOMIC: use libfam_atomic (ON) or native atomics (OFF, default)
   The flag USE_FAM_ATOMIC is not supported.

4. Test

 ```
 $ make test
 ```
 All tests should pass.


## Demo on FAME

There is a demo with two processes from two nodes. Please see the comments in
demo/demo_multi_node_alloc_free.cc for more details.

Below are the steps to run the demo:

1. Setup [FAME](https://github.hpe.com/labs/mdc-toolkit/blob/master/guide-FAME.md) with two nodes (e.g., node01 and node02)

2. Install NVMM on both nodes at directory $NVMM, with FAME support

3. Log into node01:

 ```
 $ cd $NVMM/build/demo
 $ ./demo_multi_node_alloc_free cleanup
 $ ./demo_multi_node_alloc_free init
 $ ./demo_multi_node_alloc_free runA
 ```

4. Log into node02:

 ```
 $ cd $NVMM/build/demo
 $ ./demo_multi_node_alloc_free runB
 ```

## Usage

The following code snippets illustrate how to use the Region and Heap APIs. For more details and
examples, please refer to the master source.

**Region** (direct memory mapping, see example/region_example.cc)

``` C++
#include "nvmm/error_code.h"
#include "nvmm/log.h"
#include "nvmm/memory_manager.h"

#include "nvmm/heap.h"

using namespace nvmm;

int main(int argc, char **argv)
{
    init_log(off); // turn off logging

    MemoryManager *mm = MemoryManager::GetInstance();

    // create a new 128MB NVM region with pool id 1
    PoolId pool_id = 1;
    size_t size = 128*1024*1024; // 128MB
    ErrorCode ret = mm->CreateRegion(pool_id, size);
    assert(ret == NO_ERROR);

    // acquire the region
    Region *region = NULL;
    ret = mm->FindRegion(pool_id, &region);
    assert(ret == NO_ERROR);

    // open and map the region
    ret = region->Open(O_RDWR);
    assert(ret == NO_ERROR);
    int64_t* address = NULL;
    ret = region->Map(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 0, (void**)&address);
    assert(ret == NO_ERROR);

    // use the region
    *address = 123;
    assert(*address == 123);

    // unmap and close the region
    ret = region->Unmap(address, size);
    assert(ret == NO_ERROR);
    ret = region->Close();
    assert(ret == NO_ERROR);

    // release the region
    delete region;

    // delete the region
    ret = mm->DestroyRegion(pool_id);
    assert(ret == NO_ERROR);
}
```

**Heap** (finer-grained allocation, see example/heap_example.cc)

``` C++
#include "nvmm/error_code.h"
#include "nvmm/log.h"
#include "nvmm/memory_manager.h"

#include "nvmm/heap.h"

using namespace nvmm;

int main(int argc, char **argv)
{
    init_log(off); // turn off logging

    MemoryManager *mm = MemoryManager::GetInstance();

    // create a new 128MB NVM region with pool id 2
    PoolId pool_id = 2;
    size_t size = 128*1024*1024; // 128MB
    ErrorCode ret = mm->CreateHeap(pool_id, size);
    assert(ret == NO_ERROR);

    // acquire the heap
    Heap *heap = NULL;
    ret = mm->FindHeap(pool_id, &heap);
    assert(ret == NO_ERROR);

    // open the heap
    ret =  heap->Open();
    assert(ret == NO_ERROR);

    // use the heap
    GlobalPtr ptr = heap->Alloc(sizeof(int));  // Alloc returns a GlobalPtr consisting of a shelf ID
                                               // and offset
    assert(ptr.IsValid() == true);

    int *int_ptr = (int*)mm->GlobalToLocal(ptr);  // convert the GlobalPtr into a local pointer

    *int_ptr = 123;
    assert(*int_ptr == 123);

    heap->Free(ptr);

    // close the heap
    ret = heap->Close();
    assert(ret == NO_ERROR);

    // release the heap
    delete heap;

    // delete the heap
    ret = mm->DestroyHeap(pool_id);
    assert(ret == NO_ERROR);
}
```

## Extension: Epoch-based delayed free

To provide safe sharing of allocated memory in fabric-attached memory without data copying, the
memory manager is exnteded with epochs using Fraser's algorithm, such that applications can put
memory allocation, free, and access inside epochs. The memory manager uses a delayed free method to
deallocate memory; memory will not be reclaimed until after every epoch operation in progress when
the delayed free is called has finished. The memory manager maintains per-epoch lists of memory
chunks, and the delayed free method puts a memory chunk into the corresponding per-epoch list. A
background thread wakes up periodically and frees memory chunks in those per-epoch lists. The number
of per-epoch free lists is bounded by mapping epochs to free lists in a round robin
fashion. Examples using the delayed free APIs can be found in test/allocator/test_epoch_zone_heap.cc


## Extension: Crash recovery and garbage collection

A crash during allocation, free, or merge may leak memroy and may also render the memory manager in
an inconsistent state. Necessary mechanisms are implemented to restore the memory manager state. The
memory manager provides two recovery APIs: OnlineRecover() and OfflineRecover(). OnlineRecover()
recovers from crash or failure during merge, and can run concurrently with allocation and
free. OfflineRecover(), in addition to handling merge failures, performs garbage collection to
recover leaked memory chunks. It requires exclusive access to the heap. Examples using both APIs can be found in test/allocator/test_epoch_zone_heap_crash.cc
