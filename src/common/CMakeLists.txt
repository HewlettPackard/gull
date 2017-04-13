#####################
# Yupu (March, 2016)
#
#####################

set(NVMM_SRC
  ${NVMM_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/root_shelf.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/process_id.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/nvmm_libpmem.cc
  PARENT_SCOPE
)
