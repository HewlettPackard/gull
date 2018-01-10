#####################
# Yupu (March, 2016)
#
#####################

set(NVMM_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/root_shelf.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/epoch_shelf.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/fam.c
  ${CMAKE_CURRENT_SOURCE_DIR}/process_id.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/crash_points.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/config.cc
)

if(NOT USE_FAM_ATOMIC)
set(NVMM_SRC
  ${NVMM_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/fam_atomic_x86.c
)
endif()

set(NVMM_SRC "${NVMM_SRC}" PARENT_SCOPE)
