function (add_nvmm_demo file_name)
  add_executable(${file_name} ${file_name}.cc)
  add_dependencies(${file_name} nvmm_shelf_base_dir)

  target_link_libraries(${file_name} nvmm pthread)
  target_link_libraries(${file_name} ${ARCH_LIBS})
endfunction()

add_nvmm_demo(demo_multi_node_alloc_free)
