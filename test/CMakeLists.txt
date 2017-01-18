add_subdirectory(test_common)  
#add_library(nvmm_test STATIC ${NVMM_TEST_SRC})
add_library(nvmm_test SHARED ${NVMM_TEST_SRC})
target_link_libraries(nvmm_test boost_thread boost_log boost_filesystem boost_system nvmm)

function (add_nvmm_test file_name)
  add_executable(${file_name} ${file_name}.cc)
  add_dependencies(${file_name} nvmm_shelf_base_dir)

  target_link_libraries(${file_name} nvmm_test pthread gtest)
  target_link_libraries(${file_name} ${ARCH_LIBS})
  add_test(NAME ${file_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${file_name})
endfunction()

add_subdirectory(shelf_mgmt)  
add_subdirectory(shelf_usage)  
add_subdirectory(allocator)  
add_subdirectory(memory_manager)  
