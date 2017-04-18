#####################
# Yupu (March, 2016)
#
#####################

if(ZONE)
set(NVMM_SRC
  ${NVMM_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/pool_region.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/epoch_zone_heap.cc
  PARENT_SCOPE
  )
else()
set(NVMM_SRC
  ${NVMM_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/pool_region.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/dist_heap.cc
  PARENT_SCOPE
  )
endif()
