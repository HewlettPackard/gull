add_subdirectory(common)
add_subdirectory(shelf_mgmt)
add_subdirectory(shelf_usage)
add_subdirectory(allocator)

#add_library(nvmm STATIC memory_manager.cc ${NVMM_SRC})
add_library(nvmm SHARED memory_manager.cc epoch_manager.cc ${NVMM_SRC})
target_link_libraries(nvmm boost_thread boost_log boost_log_setup boost_filesystem boost_system)
target_link_libraries(nvmm ${ARCH_LIBS})
