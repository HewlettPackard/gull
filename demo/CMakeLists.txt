function (add_nvmm_demo file_name)
  add_executable(${file_name} ${file_name}.cc)
  add_dependencies(${file_name} nvmm_shelf_base_dir)
  # the demo uses some interna:
  target_include_directories(${file_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/)
  target_link_libraries(${file_name} nvmm)
endfunction()

add_nvmm_demo(demo_multi_node_alloc_free)
