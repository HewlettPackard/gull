cmake_minimum_required(VERSION 2.8.12)
project(nvmm)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

#
# default system include and link directories
#
link_directories(/usr/lib)
include_directories(/usr/include)
link_directories(/usr/local/lib)
include_directories(/usr/local/include)


#
# build type: Release (default) or Debug
#
message(STATUS "====================")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Build type: Debug")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Build type: Release")
else()
  message(STATUS "Invalid build type: ${CMAKE_BUILD_TYPE}")
  message(STATUS "Default to : Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()
#
# determine endianness at compile time
#
include(TestBigEndian)
test_big_endian(result)
if(${result})
  add_definitions(-DSYS_BIG_ENDIAN)
  message(STATUS "Current system is big-endian")
else(${result})
  add_definitions(-DSYS_LITTLE_ENDIAN)
  message(STATUS "Current system is little-endian")
endif(${result})

#
# determine tmpfs/lfs location
#
if(FAME) # FAME
  add_definitions(-DFAME)
  set(TMPFS_PATH "/lfs")
  message(STATUS "LFS enabled")
  message(STATUS "Lfs location: ${TMPFS_PATH}")
else() # NUMA
  set(TMPFS_PATH "/dev/shm")
  message(STATUS "tmpfs location: ${TMPFS_PATH}")
endif()

#
# determine shelf base dir. Used to store files backing the shelves
#
if(FAME)
  set(SHELF_BASE_DIR "${TMPFS_PATH}")
else()
  set(SHELF_BASE_DIR "${TMPFS_PATH}/$ENV{USER}")
endif()
add_definitions(-DSHELF_USER="$ENV{USER}")
add_definitions(-DSHELF_BASE_DIR="${SHELF_BASE_DIR}")
message(STATUS "Shelf base dir: ${SHELF_BASE_DIR}")

#
# choose heap implementation
#
option(ZONE "heap implementation" ON)
if(NOT ZONE)
  message(STATUS "Heap type: hierarchical")
else()
  set(ZONE ON)
  message(STATUS "Heap type: zone")
  add_definitions(-DZONE)
endif()

#
# looking for pmem_invalidate()
#
find_library(NVML_PATH libpmem.so)
execute_process (
  COMMAND bash -c "readelf -a ${NVML_PATH} | grep pmem_invalidate"
  OUTPUT_VARIABLE result
  )
if (result STREQUAL "")
  add_definitions(-DPMEM_INVALIDATE_NOOP)
  message(STATUS "libpmem: pmem_invalidate() is noop")
else()
  message(STATUS "libpmem: pmem_invalidate() is supported")
endif()

message(STATUS "====================")

#
# common gcc flags
#
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -W -Wextra -Wredundant-decls -Wunused -Wunused-macros -Wno-unused-parameter -Wcast-qual -Wcast-align -Wwrite-strings -Wmissing-field-initializers -Wendif-labels -Winit-self -Wlogical-op -Wpacked -Wstack-protector -Wformat=2 -Wswitch-enum -Wstrict-overflow=5 -Wpointer-arith -Wnormalized=nfc -Wno-long-long -Wswitch-default -Wconversion -Wunreachable-code")  

# Flags that are currently not used: -Wshadow -Wpadded
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof ") # to suppress warnings about offsetof

#
# gcc flags for release build
#
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")

#
# gcc flags for debug build
#
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -Og -g")

#
# add libpmem and lib_fam_atomic
#
set(ARCH_LIBS pmem fam_atomic)

#
# enable GTest
#
enable_testing()

#
# enable boost
#
find_package(Boost 1.55 COMPONENTS log REQUIRED)

# for boost::log
add_definitions(-DBOOST_LOG_DYN_LINK)


#
# include directories
#
include_directories(${PROJECT_SOURCE_DIR}/include) # public headers (include)
include_directories(${PROJECT_SOURCE_DIR}/src) # private headers (src)
include_directories(${PROJECT_SOURCE_DIR}/test) # test headers (test)

#
# third-party libraries
#
set(GTEST_ROOT ${PROJECT_SOURCE_DIR}/third-party/gtest)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
add_subdirectory(third-party)
include_directories(${PROJECT_SOURCE_DIR}/third-party/gtest/include)

#
# add source code
#
add_subdirectory(src)

#
# add test code
#

# create base shelf dir
add_custom_target(
  nvmm_shelf_base_dir
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SHELF_BASE_DIR}
  COMMENT create shelf base dir for nvmm
)

add_subdirectory(test)
add_subdirectory(demo)
add_subdirectory(example)
