# Output variables:
#  FAM_ATOMIC_INCLUDE_DIR : e.g., /usr/include/.
#  FAM_ATOMIC_LIBRARY     : Library path of FAM_ATOMIC library
#  FAM_ATOMIC_FOUND       : True if found.

find_path(FAM_ATOMIC_INCLUDE_DIR NAME fam_atomic.h
  HINTS $ENV{HOME}/local/include /opt/local/include /usr/local/include /usr/include
)

find_library(FAM_ATOMIC_LIBRARY NAME fam_atomic
  HINTS $ENV{HOME}/local/lib64 $ENV{HOME}/local/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib /usr/lib64 /usr/lib
)

if (FAM_ATOMIC_INCLUDE_DIR AND FAM_ATOMIC_LIBRARY)
    set(FAM_ATOMIC_FOUND TRUE)
    message(STATUS "Found FAM_ATOMIC library: inc=${FAM_ATOMIC_INCLUDE_DIR}, lib=${FAM_ATOMIC_LIBRARY}")
else ()
    set(FAM_ATOMIC_FOUND FALSE)
    message(STATUS "WARNING: FAM_ATOMIC library not found.")
endif ()
