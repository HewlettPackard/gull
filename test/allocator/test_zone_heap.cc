#include <pthread.h>

#include <gtest/gtest.h>

#include "nvmm/memory_manager.h"
#include "allocator/zone_heap.h"

#include "test_common/test.h"

using namespace nvmm;

// single-threaded
TEST(ZoneHeap, CreateDestroyExist)
{
    PoolId pool_id = 1;
    size_t size = 128*1024*1024LLU; // 128 MB
    ZoneHeap heap(pool_id);
    
    // create a heap
    EXPECT_EQ(NO_ERROR, heap.Create(size));
    EXPECT_TRUE(heap.Exist());
    EXPECT_EQ(POOL_FOUND, heap.Create(size));

    // Destroy a heap
    EXPECT_EQ(NO_ERROR, heap.Destroy());
    EXPECT_FALSE(heap.Exist());
    EXPECT_EQ(POOL_NOT_FOUND, heap.Destroy());
}

TEST(ZoneHeap, OpenCloseSize)
{
    PoolId pool_id = 1;
    size_t size = 128*1024*1024LLU; // 128 MB
    ZoneHeap heap(pool_id);

    // open a heap that does not exist
    EXPECT_EQ(HEAP_OPEN_FAILED, heap.Open());
    
    // create a heap
    EXPECT_EQ(NO_ERROR, heap.Create(size));

    // open the heap
    EXPECT_EQ(NO_ERROR, heap.Open());

    // close the heap
    EXPECT_EQ(NO_ERROR, heap.Close());
    
    // Destroy a heap
    EXPECT_EQ(NO_ERROR, heap.Destroy());
}

TEST(ZoneHeap, AllocFreeAccess)
{
    PoolId pool_id = 1;
    GlobalPtr ptr[10];
    size_t size = 128*1024*1024LLU; // 128 MB
    ZoneHeap heap(pool_id);

    // create a heap
    EXPECT_EQ(NO_ERROR, heap.Create(size));
    
    EXPECT_EQ(NO_ERROR, heap.Open());
    for (int i=0; i<10; i++)
    {
        ptr[i] = heap.Alloc(sizeof(int));
        EXPECT_TRUE(ptr[i].IsValid());
        int *int_ptr = (int*)heap.GlobalToLocal(ptr[i]);
        *int_ptr = i;            
        
    }
    EXPECT_EQ(NO_ERROR, heap.Close());        

    EXPECT_EQ(NO_ERROR, heap.Open());    
    for (int i=0; i<10; i++)
    {
        int *int_ptr = (int*)heap.GlobalToLocal(ptr[i]);
        EXPECT_EQ(i, *int_ptr);        
        heap.Free(ptr[i]);
    }
    EXPECT_EQ(NO_ERROR, heap.Close());        

    // destroy the heap
    EXPECT_EQ(NO_ERROR, heap.Destroy());
}

// multi-threaded
struct thread_argument{
    int  id;
    ZoneHeap *heap;
    int try_count;
    GlobalPtr *ptr;
};

void *writer(void *thread_arg)
{
    thread_argument *arg = (thread_argument*)thread_arg;

    int count = arg->try_count;
    assert(count != 0);
    arg->ptr = new GlobalPtr[count];
    assert(arg->ptr != NULL);
    GlobalPtr *ptr = arg->ptr;
    
    for (int i=0; i<count; i++)
    {
        ptr[i] = arg->heap->Alloc(sizeof(int));
        if (ptr[i].IsValid() == false)
        {
            std::cout << "Thread " << arg->id << ": alloc failed" << std::endl;
        }
        else
        {
            int *int_ptr = (int*)arg->heap->GlobalToLocal(ptr[i]);
            *int_ptr = i;            
        }
    }

    pthread_exit(NULL);
}

void *reader(void *thread_arg)
{
    thread_argument *arg = (thread_argument*)thread_arg;

    int count = arg->try_count;
    assert(count != 0);

    assert(arg->ptr != NULL);    
    GlobalPtr *ptr = arg->ptr;
    
    for (int i=0; i<count; i++)
    {
        if (ptr[i].IsValid() == true)
        {
            int *int_ptr = (int*)arg->heap->GlobalToLocal(ptr[i]);
            EXPECT_EQ(i, *int_ptr);        
            arg->heap->Free(ptr[i]);
        }
    }

    delete [] ptr;
    
    pthread_exit(NULL);
}

TEST(ZoneHeap, MultiThread)
{
    int const kNumThreads = 10;
    int const kNumTry = 100;

    PoolId pool_id=1;
    size_t size = 128*1024*1024LLU; // 128 MB
    ZoneHeap heap(pool_id);

    // create a heap
    EXPECT_EQ(NO_ERROR, heap.Create(size));

    EXPECT_EQ(NO_ERROR, heap.Open());
    pthread_t threads[kNumThreads];
    thread_argument args[kNumThreads];
    int ret=0;
    void *status;
    
    for(int i=0; i<kNumThreads; i++)
    {
        //std::cout << "Create writer " << i << std::endl;
        args[i].id = i;
        args[i].heap = &heap;
        args[i].try_count = kNumTry;
        ret = pthread_create(&threads[i], NULL, writer, (void*)&args[i]);
        assert (0 == ret);
    }

    for(int i=0; i<kNumThreads; i++)
    {
        //std::cout << "Join writer " << i << std::endl;
        ret = pthread_join(threads[i], &status);
        assert (0 == ret);
    }

    EXPECT_EQ(NO_ERROR, heap.Close());        


    EXPECT_EQ(NO_ERROR, heap.Open());
    
    for(int i=0; i<kNumThreads; i++)
    {
        //std::cout << "Create reader " << i << std::endl;
        args[i].id = i;
        args[i].heap = &heap;
        args[i].try_count = kNumTry;
        ret = pthread_create(&threads[i], NULL, reader, (void*)&args[i]);
        assert (0 == ret);
    }

    for(int i=0; i<kNumThreads; i++)
    {
        //std::cout << "Join reader " << i << std::endl;
        ret = pthread_join(threads[i], &status);
        assert (0 == ret);
    }
    
    EXPECT_EQ(NO_ERROR, heap.Close());        
    // destroy the heap
    EXPECT_EQ(NO_ERROR, heap.Destroy());
}

void AllocFree(PoolId pool_id)
{
    ZoneHeap heap(pool_id);
    EXPECT_EQ(NO_ERROR, heap.Open());

    int count = 500;
    int failure_count = 0;
    size_t alloc_unit = 128*1024; // 128KB
    GlobalPtr *ptr = new GlobalPtr[count];
    assert(ptr != NULL);
    for (int i=0; i<count; i++)
    {
        ptr[i] = heap.Alloc(alloc_unit);
        if (ptr[i].IsValid() == false)
        {
            //std::cout << "Alloc failed" << std::endl;
            failure_count++;
        }
        else
        {
            int *int_ptr = (int*)heap.GlobalToLocal(ptr[i]);
            *int_ptr = i;
            //std::cout << "Alloc succeeded " << ptr[i] << std::endl;
        }
    }

    for (int i=0; i<count; i++)
    {
        if (ptr[i].IsValid() == true)
        {
            int *int_ptr = (int*)heap.GlobalToLocal(ptr[i]);
            EXPECT_EQ(i, *int_ptr);        
            heap.Free(ptr[i]);
        }
        else
        {
            //std::cout << "Invalid pointer?" << std::endl;
        }
    }
    delete [] ptr;

    std::cout << "Allocation failed: " << failure_count << " times" << std::endl;
            
    EXPECT_EQ(NO_ERROR, heap.Close());
}

TEST(ZoneHeap, MultiProcess)
{
    int const process_count = 8;

    PoolId pool_id = 1;
    size_t size = 1024*1024*1024LLU; // 1024 MB
    ZoneHeap heap(pool_id);
    // create a heap
    EXPECT_EQ(NO_ERROR, heap.Create(size));

    
    pid_t pid[process_count];

    for (int i=0; i< process_count; i++)
    {
        pid[i] = fork();
        ASSERT_LE(0, pid[i]);
        if (pid[i]==0)
        {
            // child
            AllocFree(pool_id);
            exit(0); // this will leak memory (see valgrind output)
        }
        else
        {
            // parent
            continue;
        }
    }

    for (int i=0; i< process_count; i++)
    {    
        int status;
        waitpid(pid[i], &status, 0);
    }
    
    // destroy the heap
    EXPECT_EQ(NO_ERROR, heap.Destroy());    
}

int main(int argc, char** argv)
{
    InitTest(nvmm::all, false);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


