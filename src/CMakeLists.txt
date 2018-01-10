include_directories(${PMEM_INCLUDE_DIR})

add_subdirectory(common)
add_subdirectory(shelf_mgmt)
add_subdirectory(shelf_usage)
add_subdirectory(allocator)

add_library(nvmm SHARED memory_manager.cc epoch_manager.cc ${NVMM_SRC})
target_link_libraries(nvmm ${Boost_LIBRARIES})
target_link_libraries(nvmm ${ARCH_LIBS})
target_link_libraries(nvmm yaml-cpp)
