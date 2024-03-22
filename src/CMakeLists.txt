add_library(nvmm SHARED
  memory_manager.cc
  epoch_manager.cc

  common/root_shelf.cc
  common/epoch_shelf.cc
  common/fam.c
  common/process_id.cc
  common/log.cc
  common/crash_points.cc
  common/config.cc
  $<$<NOT:$<BOOL:${USE_FAM_ATOMIC}>>:
    common/fam_atomic_x86.c>

  shelf_mgmt/shelf_file.cc
  shelf_mgmt/shelf_manager.cc
  shelf_mgmt/pool.cc
  shelf_usage/smart_shelf.cc
  shelf_usage/stack.cc
  shelf_usage/fixed_block_allocator.cc
  shelf_usage/freelists.cc
  shelf_usage/ownership.cc
  shelf_usage/shelf_region.cc
  $<IF:$<BOOL:${ZONE}>,
    shelf_usage/zone.cc
    shelf_usage/zone_entry_stack.cc
    shelf_usage/zone_shelf_heap.cc
    shelf_usage/dclcrwlock.cc
    shelf_usage/epoch_manager_impl.cc
    shelf_usage/epoch_op.cc
    shelf_usage/epoch_vector.cc
    shelf_usage/participant_manager.cc
    , # if not ZONE:
    shelf_usage/shelf_heap.cc
    >

  allocator/pool_region.cc
  $<IF:$<BOOL:${ZONE}>,
    allocator/epoch_zone_heap.cc
    , # if not ZONE:
    dist_heap.cc
    >
)

target_include_directories(nvmm PRIVATE
  .
  ${YAML_CPP_INCLUDE_DIR}  # using legacy include since config-style is rather new in yaml-cpp
)

target_include_directories(nvmm PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(nvmm PUBLIC
  Boost::log_setup
  Boost::log
)

target_link_libraries(nvmm PRIVATE
  Pmem::pmem
  Boost::thread
  Boost::system
  Boost::filesystem
  ${YAML_CPP_LIBRARIES}
)

target_compile_definitions(nvmm PRIVATE
  $<$<NOT:$<BOOL:${Pmem_HAS_PMEM_INVALIDATE}>>:PMEM_INVALIDATE_NOOP>
)
