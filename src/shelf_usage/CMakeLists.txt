#####################
# Yupu (March, 2016)
#
#####################

if(ZONE)
  set(NVMM_SRC
    ${NVMM_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/smart_shelf.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/stack.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/fixed_block_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/freelists.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ownership.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/shelf_region.cc

    ${CMAKE_CURRENT_SOURCE_DIR}/zone.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zone_entry_stack.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/zone_shelf_heap.cc

    ${CMAKE_CURRENT_SOURCE_DIR}/dclcrwlock.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/epoch_manager_impl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/epoch_op.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/epoch_vector.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/participant_manager.cc

    PARENT_SCOPE
    )
else()
  set(NVMM_SRC
    ${NVMM_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/smart_shelf.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/stack.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/fixed_block_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/freelists.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ownership.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/shelf_region.cc

    ${CMAKE_CURRENT_SOURCE_DIR}/shelf_heap.cc
    PARENT_SCOPE
    )
endif()
