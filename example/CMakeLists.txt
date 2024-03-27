function (add_nvmm_example file_name)
  add_executable(${file_name} ${file_name}.cc)
  add_dependencies(${file_name} nvmm_shelf_base_dir)

  target_link_libraries(${file_name} nvmm)
endfunction()

add_nvmm_example(region_example)
add_nvmm_example(heap_example)
add_nvmm_example(heap_resize_example)
