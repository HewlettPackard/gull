# Output variables:
#  PMEM_INCLUDE_DIR : e.g., /usr/include/.
#  PMEM_LIBRARY     : Library path of PMEM library
#  PMEM_FOUND       : True if found.

find_path(PMEM_INCLUDE_DIR NAME libpmem.h
  HINTS $ENV{HOME}/local/include /usr/local/include /usr/include /opt/usr/local/include /opt/local/include 
)

find_library(PMEM_LIBRARY NAME pmem
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /usr/lib64 /usr/lib /opt/local/lib64 /opt/local/lib /opt/usr/local/lib /opt/usr/local/lib64
)

if (PMEM_INCLUDE_DIR AND PMEM_LIBRARY)
    set(PMEM_FOUND TRUE)
    message(STATUS "Found PMEM library: inc=${PMEM_INCLUDE_DIR}, lib=${PMEM_LIBRARY}")
else ()
    set(PMEM_FOUND FALSE)
    message(STATUS "WARNING: PMEM library not found.")
endif ()

# Check if pmem_invalidate is supported
if (PMEM_FOUND)
  execute_process (
    COMMAND bash -c "readelf -a ${PMEM_LIBRARY} | grep pmem_invalidate"
    OUTPUT_VARIABLE result
  )
  if (result STREQUAL "")
    set(PMEM_HAS_PMEM_INVALIDATE FALSE)
    message(STATUS "libpmem: pmem_invalidate() is noop")
  else()
    set(PMEM_HAS_PMEM_INVALIDATE TRUE)
    message(STATUS "libpmem: pmem_invalidate() is supported")
  endif()
endif()
